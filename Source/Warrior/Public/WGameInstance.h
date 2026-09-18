// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "SaveGame/WSaveGame.h"
#include "WGameInstance.generated.h"

USTRUCT(BlueprintType)
struct FWGameLevelSet
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, meta = (Categories = "GameData.Level"))
	FGameplayTag LevelTag;

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> Level;

	bool IsValid() const
	{
		return LevelTag.IsValid() && !Level.IsNull();
	}
};

/**
 *
 */
UCLASS(Abstract)
class WARRIOR_API UWGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

protected:

	virtual void OnPreLoadMap(const FString& MapName);
	virtual void OnDestinationWorldLoaded(UWorld* LoadedWorld);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FWGameLevelSet> GameLevelSets;

public:
	UFUNCTION(BlueprintPure, meta=(GameplayTagFilter = "GameData.Level"))
	TSoftObjectPtr<UWorld> GetGameLevelByTag(FGameplayTag InTag) const;

	// 由主菜单「继续游戏」按钮置 true：进入生存关卡时从存档恢复波数/难度；
	// 「新游戏」按钮应将其置 false（或不改，默认为 false）。
	UPROPERTY(BlueprintReadWrite, Category = "Game Settings")
	bool bWantsContinue = false;

	// 读档后由 GameMode 填充，等 HeroCharacter 出生时消费，用于恢复角色成长属性/装备。
	// 仅在 bWantsContinue == true 且确有存档时置 true，消费后由 HeroCharacter 清回 false。
	UPROPERTY(BlueprintReadWrite, Category = "Game Settings")
	FWPlayerSaveData PendingLoadData;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings")
	bool bHasPendingPlayerData = false;
};
