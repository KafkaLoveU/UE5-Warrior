// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "WTypes/WEnumTypes.h"
#include "WGameplayAbility.generated.h"

struct FWGameplayAbilityActorInfo;
class UWAbilitySystemComponent;
class UPawnCombatComponent;

UENUM(BlueprintType)
enum class EWAbilityActivationPolicy : uint8
{
	OnTriggered,
	OnGiven,
};

UCLASS(Abstract)
class WARRIOR_API UWGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

protected:

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	UPROPERTY(EditDefaultsOnly)
	EWAbilityActivationPolicy AbilityActivationPolicy = EWAbilityActivationPolicy::OnTriggered;

	UFUNCTION(BlueprintPure)
	UPawnCombatComponent* GetPawnCombatComponentFromActorInfo() const;

	UFUNCTION(BlueprintPure)
	UWAbilitySystemComponent* GetWAbilitySystemComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable)
	void ApplyEffectSpecHandleToHitResult(const FGameplayEffectSpecHandle& InSpecHandle, const TArray<FHitResult>& InHitResults);

	FActiveGameplayEffectHandle ApplyEffectSpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Aplly Effect SpecHandle to Target", ExpandEnumAsExecs = "OutSuccessType"))
	FActiveGameplayEffectHandle BP_ApplyEffectSpecHandleToTarget(AActor* TargetActor, const FGameplayEffectSpecHandle& InSpecHandle, EWSuccessType& OutSuccessType);

	const FWGameplayAbilityActorInfo* GetWActorInfo(const FGameplayAbilityActorInfo* InInfo) const;
};