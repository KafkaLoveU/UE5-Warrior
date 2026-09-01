// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Combat/HeroCombatComponent.h"
#include "WGameplayTags.h"
#include "Items/Weapons/WHeroWeapon.h"
#include "AbilitySystemBlueprintLibrary.h"

AWHeroWeapon* UHeroCombatComponent::GetHeroCarriedWeaponByTag(FGameplayTag InWeaponTag) const
{
	return Cast<AWHeroWeapon>(GetCharacterCarriedWeaponByTag(InWeaponTag));
}

AWHeroWeapon* UHeroCombatComponent::GetHeroCurrentEquippedWeapon() const
{
	return Cast<AWHeroWeapon>(GetCharacterCurrentEquippedWeapon());
}

float UHeroCombatComponent::GetHeroCurrentEquipWeaponDamageAtLevel(float InLevel) const
{
	const AWHeroWeapon* CurrentEquippedWeapon = GetHeroCurrentEquippedWeapon();
	return CurrentEquippedWeapon ? CurrentEquippedWeapon->HeroWeaponData.WeaponBaseDamage.GetValueAtLevel(InLevel) : 0.f;
}

void UHeroCombatComponent::OnHitTargetActor(AActor* HitActor)
{
	if (OverlappedActors.Contains(HitActor))
	{
		return;
	}

	OverlappedActors.AddUnique(HitActor);

	FGameplayEventData Data;
	Data.Instigator = GetOwningPawn();
	Data.Target = HitActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwningPawn(), WTags::Shared_Event_MeleeHit, Data);

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwningPawn(), WTags::Player_Event_HitPause, FGameplayEventData());
}

void UHeroCombatComponent::OnWeaponPulledFromTargetActor(AActor* InteractedActor)
{
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwningPawn(), WTags::Player_Event_HitPause, FGameplayEventData());
}