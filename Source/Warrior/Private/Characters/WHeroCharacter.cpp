// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/WHeroCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameMode/WBaseGameMode.h"
#include "DataAssets/StartUpData/DataAsset_HeroStartUpData.h"
#include "Components/UI/HeroUIComponent.h"
#include "Components/UI/PawnUIComponent.h"
#include "Components/Combat/HeroCombatComponent.h"
#include "EnhancedInputSubsystems.h"
#include "DataAssets/Input/DataAsset_InputConfig.h"
#include "WGameplayTags.h"
#include "Components/Input/WInputComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/WAbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/WAttributeSet.h"
#include "WGameInstance.h"
#include "SaveGame/WSaveGame.h"

AWHeroCharacter::AWHeroCharacter()
{
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 200.f;
	CameraBoom->SocketOffset = FVector(0.f, 55.f, 65.f);
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 500.f, 0.f);
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;

	HeroCombatComponent = CreateDefaultSubobject<UHeroCombatComponent>("HeroCombatComponent");

	HeroUIComponent = CreateDefaultSubobject<UHeroUIComponent>("HeroUIComponent");
}

UPawnCombatComponent* AWHeroCharacter::GetPawnCombatComponent() const
{
	return HeroCombatComponent;
}

UPawnUIComponent* AWHeroCharacter::GetPawnUIComponent() const
{
	return HeroUIComponent;
}

UHeroUIComponent* AWHeroCharacter::GetHeroUIComponent() const
{
	return HeroUIComponent;
}

void AWHeroCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (!CharacterStartUpData.IsNull())
	{
		if (UDataAsset_StartUpDataBase* LoadedData = CharacterStartUpData.LoadSynchronous())
		{
			int32 AbilityApplyLevel = 1;

			if (const AWBaseGameMode* BaseGameMode = GetWorld()->GetAuthGameMode<AWBaseGameMode>())
			{
				switch (BaseGameMode->GetCurrentGameDifficulty())
				{
				case EWGameDifficulty::Easy: AbilityApplyLevel = 4; break;
				case EWGameDifficulty::Normal: AbilityApplyLevel = 3; break;
				case EWGameDifficulty::Hard: AbilityApplyLevel = 2; break;
				case EWGameDifficulty::VeryHard: AbilityApplyLevel = 1; break;
				}
			}

			LoadedData->GiveToAbilitySystemComponent(WAbilitySystemComponent, AbilityApplyLevel);
		}
	}

	// 续关：角色被 Possess 后，若有待还原的装备武器，触发 Equip 能力重新生成武器外观
	TryRestoreContinueEquip();
}

void AWHeroCharacter::TryRestoreContinueEquip()
{
	if (!bPendingContinueEquip || !PendingContinueEquipWeaponTag.IsValid())
	{
		return;
	}

	// 角色尚未被 Possess 时，AnimInstance/Controller 未就绪，等 PossessedBy 再调用本函数
	if (!GetController())
	{
		return;
	}

	const FGameplayTag EquippedTag = PendingContinueEquipWeaponTag;

	// 续关还原武器：完全复刻"玩家手动按一次装备键"的完整路径——调用 OnAbilityInputPressed(Input.EquipAxe)，
	// 其内部对匹配 DynamicAbilityTags 的装备能力执行 TryActivateAbility，与正常游戏手动按键走完全一致的逻辑
	//（能力自身负责 Spawn 武器、播放 AM_Hero_Axe_Equip montage、由 AnimNotify 发送
	//  Player.Event.Equip.Axe 把武器从背上拔到手上、并设置 CurrentEquippedWeaponTag）。
	// 注意：这里【不要】手动调用 SetCurrentEquipWeaponByTag / SendGameplayEvent，
	// 否则会绕过或覆盖能力自身的装备流程，导致武器停在背上（之前多版的根因）。
	FTimerDelegate RestoreWeaponDelegate;
	RestoreWeaponDelegate.BindWeakLambda(this, [this, EquippedTag]()
	{
		if (!IsValid(this))
		{
			return;
		}

		UWAbilitySystemComponent* WASC = Cast<UWAbilitySystemComponent>(GetAbilitySystemComponent());
		UHeroCombatComponent* HC = GetHeroCombatComponent();
		if (!WASC || !HC)
		{
			return;
		}

		// 判断是否已“拔刀在手上”：CurrentEquippedWeaponTag 被装备能力设为斧头标签即代表已装备。
		// 续关时武器可能已自动 Spawn 并注册（CharacterCarriedWeaponMap 有），但处于“挂在背上/未拔”状态，
		// 此时 CurrentEquippedWeaponTag 不是斧头标签，需要触发装备能力的拔刀逻辑把它拔到手上。
		auto IsDrawn = [&]() { return HC->CurrentEquippedWeaponTag == EquippedTag; };

		if (IsDrawn())
		{
			UE_LOG(LogTemp, Log, TEXT("[ContinueEquip] %s already drawn in hand, nothing to do"), *EquippedTag.ToString());
			return;
		}

		// 触发一次装备键：若武器尚未 Spawn 则生成，若已 Spawn 但挂在背上则拔刀（与玩家手动按键完全一致）。
		WASC->OnAbilityInputPressed(WTags::Input_EquipAxe);
		UE_LOG(LogTemp, Log, TEXT("[ContinueEquip] Pressed EquipAxe for %s; carried=%s drawn=%s"),
			*EquippedTag.ToString(),
			HC->GetCharacterCarriedWeaponByTag(EquippedTag) ? TEXT("yes") : TEXT("no"),
			IsDrawn() ? TEXT("yes") : TEXT("no"));

		// 0.3s 后校验兜底：若武器尚未生成（首次仅 Spawn）或仍未拔刀，再触发一次装备键确保最终在手上。
		FTimerHandle VerifyTimer;
		GetWorldTimerManager().SetTimer(VerifyTimer, FTimerDelegate::CreateWeakLambda(this, [this, EquippedTag, WASC, HC]()
		{
			if (!IsValid(this))
			{
				return;
			}

			if (!HC->GetCharacterCarriedWeaponByTag(EquippedTag))
			{
				WASC->OnAbilityInputPressed(WTags::Input_EquipAxe);
				UE_LOG(LogTemp, Log, TEXT("[ContinueEquip] Weapon not spawned yet, pressed EquipAxe again to spawn+draw %s"), *EquippedTag.ToString());
				return;
			}

			if (HC->CurrentEquippedWeaponTag != EquippedTag)
			{
				WASC->OnAbilityInputPressed(WTags::Input_EquipAxe);
				UE_LOG(LogTemp, Log, TEXT("[ContinueEquip] Still not drawn, pressed EquipAxe again to draw %s"), *EquippedTag.ToString());
			}
		}), 0.3f, false);
	});
	FTimerHandle RestoreWeaponTimer;
	GetWorldTimerManager().SetTimer(RestoreWeaponTimer, RestoreWeaponDelegate, 0.5f, false);

	bPendingContinueEquip = false;
}


void AWHeroCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	checkf(InputConfigDataAsset, TEXT("Forgot to assign a valid data asset as input config"));

	const ULocalPlayer* LocalPlayer = GetController<APlayerController>()->GetLocalPlayer();

	UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer);

	check(Subsystem);

	Subsystem->AddMappingContext(InputConfigDataAsset->DefaultMappingContext,0);

	UWInputComponent* WInputComponent = CastChecked<UWInputComponent>(PlayerInputComponent);

	WInputComponent->BindNativeInputAction(InputConfigDataAsset, WTags::Input_Move, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	WInputComponent->BindNativeInputAction(InputConfigDataAsset, WTags::Input_Look, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	
	WInputComponent->BindNativeInputAction(InputConfigDataAsset, WTags::Input_SwitchTarget, ETriggerEvent::Triggered, this, &ThisClass::Input_SwitchTargetTriggered);
	WInputComponent->BindNativeInputAction(InputConfigDataAsset, WTags::Input_SwitchTarget, ETriggerEvent::Completed, this, &ThisClass::Input_SwitchTargetCompleted);

	WInputComponent->BindNativeInputAction(InputConfigDataAsset, WTags::Input_Pickup_Stones, ETriggerEvent::Started, this, &ThisClass::Input_PickupStonesStarted);
	
	WInputComponent->BindAbilityInputAction(InputConfigDataAsset, this, &ThisClass::Input_AbilityInputPressed, &ThisClass::Input_AbilityInputReleased);
}

void AWHeroCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 续关：从 GameInstance 的待恢复数据还原角色成长属性与装备
	if (UWGameInstance* GameInstance = GetGameInstance<UWGameInstance>())
	{
		if (GameInstance->bWantsContinue && GameInstance->bHasPendingPlayerData)
		{
			const FWPlayerSaveData& Data = GameInstance->PendingLoadData;

			// 还原属性（用 ASC 实际注册的 AttributeSet，不要用基类成员指针）
			if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				if (UWAttributeSet* AS = const_cast<UWAttributeSet*>(ASC->GetSet<UWAttributeSet>()))
				{
					AS->SetMaxHealth(Data.MaxHealth);
					AS->SetMaxRage(Data.MaxRage);
					AS->SetAttackPower(Data.AttackPower);
					AS->SetDefensePower(Data.DefensePower);
					AS->SetCurrentHealth(Data.CurrentHealth); // 还原存档瞬间的真实血量
					AS->SetCurrentRage(Data.CurrentRage);      // 还原存档瞬间的真实怒气

					// 直接 SetCurrentHealth 不会触发 PostGameplayEffectExecute，UMG 不会刷新，
					// 因此手动广播一次血量/怒气变化，让 HUD 立即显示存档值。
					if (UPawnUIComponent* PawnUI = GetPawnUIComponent())
					{
						PawnUI->OnCurrentHealthChanged.Broadcast(AS->GetCurrentHealth() / AS->GetMaxHealth());
					}
					if (UHeroUIComponent* HeroUI = GetHeroUIComponent())
					{
						HeroUI->OnCurrentRageChanged.Broadcast(AS->GetCurrentRage() / AS->GetMaxRage());
					}
				}
			}

			// 还原装备：武器生成逻辑封装在蓝图装备能力 Player.Ability.Equip.Axe 中。
			// 续关时正常开局流程未执行装备，CharacterCarriedWeaponMap 为空，
			// 因此不能在这里（BeginPlay 可能早于 Possess）立即激活 Equip 能力，
			// 否则 AnimInstance/Controller 未就绪会导致武器 Attach 到错误 Socket（姿势异常）。
			// 改为记录待装备 Tag，等角色被 Possess 后由 TryRestoreContinueEquip() 触发。
			if (Data.EquippedWeaponTag.IsValid())
			{
				PendingContinueEquipWeaponTag = Data.EquippedWeaponTag;
				bPendingContinueEquip = true;
				TryRestoreContinueEquip();
			}

			GameInstance->bHasPendingPlayerData = false;
		}
	}
}

void AWHeroCharacter::Input_Move(const FInputActionValue& InputActionValue)
{
	const FVector2D MovementVector = InputActionValue.Get<FVector2D>();

	const FRotator MovementRotation(0.f, Controller->GetControlRotation().Yaw, 0.f);

	if (MovementVector.Y != 0.f || MovementVector.X != 0.f)
	{
		const FVector ForwardDirection = MovementRotation.RotateVector(FVector::ForwardVector);
		const FVector RightDirection = MovementRotation.RotateVector(FVector::RightVector);

		const FVector MovementDirection = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);

		AddMovementInput(MovementDirection, 1.f);
	}
}

void AWHeroCharacter::Input_Look(const FInputActionValue& InputActionValue)
{
	const FVector2D LookAxisVector = InputActionValue.Get<FVector2D>();

	if (LookAxisVector.X != 0.f || LookAxisVector.Y != 0.f)
	{
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AWHeroCharacter::Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue)
{
	SwitchDirection = InputActionValue.Get<FVector2D>();
}

void AWHeroCharacter::Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue)
{
	FGameplayEventData Data;
	FGameplayTag EventTag = SwitchDirection.X > 0.f ? WTags::Player_Event_SwitchTarget_Right : WTags::Player_Event_SwitchTarget_Left;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Data);
}

void AWHeroCharacter::Input_PickupStonesStarted(const FInputActionValue& InpuActionValue)
{
	FGameplayEventData Data;
	FGameplayTag EventTag = WTags::Player_Event_ConsumeStones;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, EventTag, Data);
}

void AWHeroCharacter::Input_AbilityInputPressed(FGameplayTag InInputTag)
{
	if (!ensure(WAbilitySystemComponent)) return;
	WAbilitySystemComponent->OnAbilityInputPressed(InInputTag);
}

void AWHeroCharacter::Input_AbilityInputReleased(FGameplayTag InInputTag)
{
	if (!ensure(WAbilitySystemComponent)) return;
	WAbilitySystemComponent->OnAbilityInputReleased(InInputTag);
}