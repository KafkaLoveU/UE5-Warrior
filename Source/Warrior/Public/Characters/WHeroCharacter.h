// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Characters/WBaseCharacter.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "WHeroCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UDataAsset_InputConfig;
struct FInputActionValue;
class UHeroCombatComponent;
class UHeroUIComponent;
class UWInventoryComponent;
class UWInventoryWidget;

UCLASS()
class WARRIOR_API AWHeroCharacter : public AWBaseCharacter
{
	GENERATED_BODY()

public:
	AWHeroCharacter();

	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;

	virtual UPawnUIComponent* GetPawnUIComponent() const override;
	virtual UHeroUIComponent* GetHeroUIComponent() const override;

protected:
	virtual void PossessedBy(AController* NewController) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void BeginPlay() override;
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PrevCustomMode) override;

private:
#pragma region Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|Camera", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeroCombatComponent> HeroCombatComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UHeroUIComponent> HeroUIComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWInventoryComponent> InventoryComponent;

	// 背包 UI 的 Widget Blueprint 类（在英雄 Blueprint 的 Class Defaults 里指定，如 WBP_Inventory）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default|Inventory", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWInventoryWidget> InventoryWidgetClass;

	// 运行时创建的背包 Widget 实例（BeginPlay 中生成并加入视口，默认隐藏）
	UPROPERTY()
	TObjectPtr<UWInventoryWidget> InventoryWidget;
#pragma endregion

#pragma region Inputs
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default|CharacterData", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UDataAsset_InputConfig> InputConfigDataAsset;

	UPROPERTY()
	FVector2D SwitchDirection = FVector2D::ZeroVector;

	void Input_Move(const FInputActionValue& InputActionValue);
	void Input_Look(const FInputActionValue& InputActionValue);
	void Input_SwitchTargetTriggered(const FInputActionValue& InputActionValue);
	void Input_SwitchTargetCompleted(const FInputActionValue& InputActionValue);
	void Input_PickupStonesStarted(const FInputActionValue& InputActionValue);
	void Input_ToggleInventory(const FInputActionValue& InputActionValue);

	void Input_AbilityInputPressed(FGameplayTag InInputTag);
	void Input_AbilityInputReleased(FGameplayTag InInputTag);
#pragma endregion

#pragma region Continue Save
	// 续关：记录需要还原的装备武器 Tag，等角色被 Possess 后再触发 Equip 能力，
	// 避免 BeginPlay 过早激活导致 AnimInstance/Controller 未就绪、武器姿势异常
	FGameplayTag PendingContinueEquipWeaponTag;
	bool bPendingContinueEquip = false;
	void TryRestoreContinueEquip();
#pragma endregion

public:
	FORCEINLINE UHeroCombatComponent* GetHeroCombatComponent() const { return HeroCombatComponent; }
	FORCEINLINE UWInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }
	FORCEINLINE UWInventoryWidget* GetInventoryWidget() const { return InventoryWidget; }
};
