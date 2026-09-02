// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystemInterface.h"
#include "Interfaces/PawnCombatInterface.h"
#include "Interfaces/PawnUIInterface.h"
#include "GameFramework/Character.h"

#include "WBaseCharacter.generated.h"

class UWAbilitySystemComponent;
class UWAttributeSet;
class UDataAsset_StartUpDataBase;
class UMotionWarpingComponent;

UCLASS(Abstract)
class WARRIOR_API AWBaseCharacter : public ACharacter, public IAbilitySystemInterface, public IPawnCombatInterface, public IPawnUIInterface
{
	GENERATED_BODY()

public:
	AWBaseCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	virtual UPawnCombatComponent* GetPawnCombatComponent() const override;

	virtual UPawnUIComponent* GetPawnUIComponent() const override;

protected:
	virtual void PossessedBy(AController* NewController) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|AbilitySystem")
	TObjectPtr<UWAbilitySystemComponent> WAbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|AbilitySystem")
	TObjectPtr<UWAttributeSet> WAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Default|AbilitySystem")
	TSoftObjectPtr<UDataAsset_StartUpDataBase> CharacterStartUpData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Default|AbilitySystem")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

public:
	FORCEINLINE UWAbilitySystemComponent* GetWAbilitySystemComponent() const
	{
		return WAbilitySystemComponent;
	}

	FORCEINLINE UWAttributeSet* GetWAttributeSet() const
	{
		return WAttributeSet;
	}
};