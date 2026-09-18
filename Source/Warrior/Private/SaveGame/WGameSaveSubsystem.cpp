// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveGame/WGameSaveSubsystem.h"

#include "SaveGame/WSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "WGameplayTags.h"

void UWGameSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

FString UWGameSaveSubsystem::GetSlotName() const
{
	return WTags::GameData_SaveGame_Slot_1.GetTag().ToString();
}

void UWGameSaveSubsystem::SaveGame(const FWPlayerSaveData& InData)
{
	// 关键：先尝试读旧档，没有再新建 —— 否则多字段存档会被互相覆盖
	UWSaveGame* SaveObject = nullptr;

	if (UGameplayStatics::DoesSaveGameExist(GetSlotName(), 0))
	{
		SaveObject = Cast<UWSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(), 0));
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

	UGameplayStatics::SaveGameToSlot(SaveObject, GetSlotName(), 0);
}

bool UWGameSaveSubsystem::TryLoadGame(FWPlayerSaveData& OutData)
{
	if (!HasSave())
	{
		return false;
	}

	if (UWSaveGame* SaveObject = Cast<UWSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(), 0)))
	{
		OutData = SaveObject->PlayerData;
		return true;
	}

	return false;
}

bool UWGameSaveSubsystem::HasSave() const
{
	return UGameplayStatics::DoesSaveGameExist(GetSlotName(), 0);
}

void UWGameSaveSubsystem::DeleteSave()
{
	if (HasSave())
	{
		UGameplayStatics::DeleteGameInSlot(GetSlotName(), 0);
	}
}
