// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveGame/WGameSaveSubsystem.h"

#include "SaveGame/WSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "WGameplayTags.h"

void UWGameSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

FString UWGameSaveSubsystem::GetSlotName(int32 SlotIndex)
{
	SlotIndex = FMath::Clamp(SlotIndex, 0, MaxSaveSlots - 1);
	switch (SlotIndex)
	{
		case 1: return WTags::GameData_SaveGame_Slot_2.GetTag().ToString();
		case 2: return WTags::GameData_SaveGame_Slot_3.GetTag().ToString();
		default: return WTags::GameData_SaveGame_Slot_1.GetTag().ToString();
	}
}

void UWGameSaveSubsystem::SaveGame(const FWPlayerSaveData& InData, int32 SlotIndex)
{
	// 关键：先尝试读旧档，没有再新建 —— 否则多字段存档会被互相覆盖
	const FString SlotName = GetSlotName(SlotIndex);
	UWSaveGame* SaveObject = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		SaveObject = Cast<UWSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}

	if (!SaveObject)
	{
		SaveObject = Cast<UWSaveGame>(UGameplayStatics::CreateSaveGameObject(UWSaveGame::StaticClass()));
	}

	if (!SaveObject)
	{
		return;
	}

	SaveObject->SaveVersion = CurrentSaveVersion;
	SaveObject->PlayerData = InData;

	UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
}

bool UWGameSaveSubsystem::TryLoadGame(FWPlayerSaveData& OutData, int32 SlotIndex)
{
	const FString SlotName = GetSlotName(SlotIndex);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		return false;
	}

	if (UWSaveGame* SaveObject = Cast<UWSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
	{
		OutData = SaveObject->PlayerData;
		return true;
	}

	return false;
}

bool UWGameSaveSubsystem::HasSave(int32 SlotIndex) const
{
	return UGameplayStatics::DoesSaveGameExist(GetSlotName(SlotIndex), 0);
}

void UWGameSaveSubsystem::DeleteSave(int32 SlotIndex)
{
	const FString SlotName = GetSlotName(SlotIndex);
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	}
}
