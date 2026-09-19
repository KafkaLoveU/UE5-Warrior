// Fill out your copyright notice in the Description page of Project Settings.

#include "WFunctionLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/WAbilitySystemComponent.h"
#include "Interfaces/PawnCombatInterface.h"
#include "Controllers/WHeroController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "WGameplayTags.h"
#include "WTypes/WCountdownAction.h"
#include "WGameInstance.h" 
#include "SaveGame/WSaveGame.h"
#include "SaveGame/WGameSaveSubsystem.h"


UWAbilitySystemComponent* UWFunctionLibrary::GetWarriorASCFromActor(const AActor* InActor)
{
	return Cast<UWAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor));
}

void UWFunctionLibrary::AddGameplayTagToActorIfNone(AActor* InActor, FGameplayTag TagToAdd)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	ensure(ASC);

	if (ASC && ASC->HasMatchingGameplayTag(TagToAdd) == false)
	{
		ASC->AddLooseGameplayTag(TagToAdd);
	}
}

void UWFunctionLibrary::RemoveGameplayTagFromActorIfFound(AActor* InActor, FGameplayTag TagToRemove)
{
	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	ensure(ASC);

	if (ASC && ASC->HasMatchingGameplayTag(TagToRemove))
	{
		ASC->RemoveLooseGameplayTag(TagToRemove);
	}
}

bool UWFunctionLibrary::DoesActorHaveTag(const AActor* InActor, FGameplayTag TagToCheck)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InActor);
	ensure(ASC);

	return ASC ? ASC->HasMatchingGameplayTag(TagToCheck) : false;
}

void UWFunctionLibrary::BP_DoesActorHaveTag(const AActor* InActor, FGameplayTag TagToCheck, EWConfirmType& OutConfirmType)
{
	if (!ensure(InActor))
	{
		OutConfirmType = EWConfirmType::No;
		return;
	}
	OutConfirmType = DoesActorHaveTag(InActor, TagToCheck) ? EWConfirmType::Yes : EWConfirmType::No;
}

UPawnCombatComponent* UWFunctionLibrary::GetPawnCombatComponent(const AActor* InActor)
{
	ensure(InActor);

	if (const IPawnCombatInterface* PawnCombatInterface = Cast<IPawnCombatInterface>(InActor))
	{
		return PawnCombatInterface->GetPawnCombatComponent();
	}

	return nullptr;
}

UPawnCombatComponent* UWFunctionLibrary::BP_GetPawnCombatComponentFromActor(const AActor* InActor, EWValidType& OutValidType)
{
	UPawnCombatComponent* CombatComponent = GetPawnCombatComponent(InActor);

	OutValidType = CombatComponent ? EWValidType::Valid : EWValidType::Invalid;

	return CombatComponent;
}

bool UWFunctionLibrary::IsTargetPawnHostile(const APawn* QueryPawn, const APawn* TargetPawn)
{
	if (!ensure(QueryPawn && TargetPawn))
	{
		return false;
	}

	const IGenericTeamAgentInterface* QueryTeamAgent = Cast<IGenericTeamAgentInterface>(QueryPawn->GetController());
	const IGenericTeamAgentInterface* TargetTeamAgent = Cast<IGenericTeamAgentInterface>(TargetPawn->GetController());

	if (QueryTeamAgent && TargetTeamAgent)
	{
		return QueryTeamAgent->GetGenericTeamId() != TargetTeamAgent->GetGenericTeamId();
	}

	return false;
}

float UWFunctionLibrary::GetScalableFloatValueAtLevel(const FScalableFloat& InScalableFloat, float InLevel)
{
	return InScalableFloat.GetValueAtLevel(InLevel);
}

FGameplayTag UWFunctionLibrary::ComputeHitReactDirectionTag(const AActor* InAttacker, const AActor* InVictim, float& OutAngleDifference)
{
	if (!ensure(InAttacker && InVictim))
	{
		return FGameplayTag::EmptyTag;
	}

	const FVector VictimForward = InVictim->GetActorForwardVector();
	const FVector VictimToAttackerNormalize = (InAttacker->GetActorLocation() - InVictim->GetActorLocation()).GetSafeNormal();

	const float DotResult = FVector::DotProduct(VictimForward, VictimToAttackerNormalize);
	OutAngleDifference = UKismetMathLibrary::DegAcos(DotResult);

	const FVector CrossResult = FVector::CrossProduct(VictimForward, VictimToAttackerNormalize);

	if (CrossResult.Z < 0.f)
	{
		OutAngleDifference *= -1.f;
	}

	if (OutAngleDifference >= -45.f && OutAngleDifference <= 45.f)
	{
		return WTags::Shared_Status_HitReact_Front;
	}
	else if (OutAngleDifference < -45.f && OutAngleDifference >= -135.f)
	{
		return WTags::Shared_Status_HitReact_Left;
	}
	else if (OutAngleDifference < -135.f || OutAngleDifference > 135.f)
	{
		return WTags::Shared_Status_HitReact_Back;
	}
	else if (OutAngleDifference > 45.f && OutAngleDifference <= 135.f)
	{
		return WTags::Shared_Status_HitReact_Right;
	}

	return WTags::Shared_Status_HitReact_Front;
}

bool UWFunctionLibrary::IsValidBlock(const AActor* InAttacker, const AActor* InDefender)
{
	if (!ensure(InAttacker && InDefender))
	{
		return  false;
	}

	const float DotResult = FVector::DotProduct(InAttacker->GetActorForwardVector(), InDefender->GetActorForwardVector());

	return DotResult < -0.1f;
}

bool UWFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(AActor* InInstigator,AActor* InTargetActor,const FGameplayEffectSpecHandle& InSpecHandle)
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InInstigator);
	UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InTargetActor);

	FActiveGameplayEffectHandle ActiveHandle = SourceASC->ApplyGameplayEffectSpecToTarget(*InSpecHandle.Data, TargetASC);

	return ActiveHandle.WasSuccessfullyApplied();
}

void UWFunctionLibrary::CountDown(const UObject* WorldContextObject, float TotalTime, float UpdateInterval, float& OutRemainingTime, EWCountdownActionInput CountdownInput, UPARAM(DisplayName = "Output") EWCountdownActionOutput& CountdownOutput, FLatentActionInfo LatentInfo)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;
	if (!World)
	{
		return;
	}

	FLatentActionManager& LatentActionManager = World->GetLatentActionManager();

	FWCountdownAction* FoundAction = LatentActionManager.FindExistingAction<FWCountdownAction>(LatentInfo.CallbackTarget, LatentInfo.UUID);

	if (CountdownInput == EWCountdownActionInput::Start)
	{
		if (!FoundAction)
		{
			LatentActionManager.AddNewAction(
				LatentInfo.CallbackTarget,
				LatentInfo.UUID,
				new FWCountdownAction(TotalTime, UpdateInterval, OutRemainingTime, CountdownOutput, LatentInfo));
		}
	}

	if (CountdownInput == EWCountdownActionInput::Cancel)
	{
		if (FoundAction)
		{
			FoundAction->CancelAction();
		}
	}
}

UWGameInstance* UWFunctionLibrary::GetWGameInstance(const UObject* WorldContextObject)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	return World ? World->GetGameInstance<UWGameInstance>() : nullptr;
}

void UWFunctionLibrary::ToggleInputMode(const UObject* WorldContextObject, EWInputMode InInputMode)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull) : nullptr;

	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;

	if (!PlayerController)
	{
		return;
	}

	FInputModeUIOnly UIOnlyMode;
	FInputModeGameOnly GameOnlyMode;

	switch (InInputMode)
	{
	case EWInputMode::GameOnly:
		PlayerController->SetInputMode(GameOnlyMode);
		PlayerController->bShowMouseCursor = false;
		break;

	case EWInputMode::UIOnly:
		PlayerController->SetInputMode(UIOnlyMode);
		PlayerController->bShowMouseCursor = true;
		break;
	}
}

void UWFunctionLibrary::SaveCurrentGameDifficulty(EWGameDifficulty InDifficultyToSave, int32 SlotIndex)
{
	// B 档后难度并入 UWSaveGame::PlayerData。这里只更新难度字段，
	// 先从已有存档读回完整 PlayerData 再写回，避免覆盖 B 档统一存档（波数/属性/装备）。
	const FString SlotName = UWGameSaveSubsystem::GetSlotName(SlotIndex);

	UWSaveGame* WSaveGameObject = nullptr;
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		WSaveGameObject = Cast<UWSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}
	if (!WSaveGameObject)
	{
		WSaveGameObject = Cast<UWSaveGame>(UGameplayStatics::CreateSaveGameObject(UWSaveGame::StaticClass()));
	}

	if (WSaveGameObject)
	{
		WSaveGameObject->PlayerData.Difficulty = InDifficultyToSave;
		UGameplayStatics::SaveGameToSlot(WSaveGameObject, SlotName, 0);
	}
}

bool UWFunctionLibrary::TryLoadSavedGameDifficulty(EWGameDifficulty& OutSavedDifficulty, int32 SlotIndex)
{
	const FString SlotName = UWGameSaveSubsystem::GetSlotName(SlotIndex);

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		USaveGame* SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, 0);

		if (UWSaveGame* WSaveGameObject = Cast<UWSaveGame>(SaveGameObject))
		{
			OutSavedDifficulty = WSaveGameObject->PlayerData.Difficulty;

			return true;
		}
	}

	return false;
}