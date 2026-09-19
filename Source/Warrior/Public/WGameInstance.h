// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "GameplayTagContainer.h"
#include "SaveGame/WSaveGame.h"
#include "WGameInstance.generated.h"

class UWGameSaveSubsystem;

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

	// 多存档槽（C 方案）：主菜单选定从/向哪个槽位读写。0=槽1, 1=槽2, 2=槽3。
	// 点「继续」时设为对应槽并 bWantsContinue=true；新游戏时设为对应槽、bWantsContinue=false，
	// 之后 InitGame 读档与游戏内 SaveCurrentProgress 落盘都走这个槽。
	UPROPERTY(BlueprintReadWrite, Category = "Game Settings")
	int32 ActiveSaveSlot = 0;

	// 读档后由 GameMode 填充，等 HeroCharacter 出生时消费，用于恢复角色成长属性/装备。
	// 仅在 bWantsContinue == true 且确有存档时置 true，消费后由 HeroCharacter 清回 false。
	UPROPERTY(BlueprintReadWrite, Category = "Game Settings")
	FWPlayerSaveData PendingLoadData;

	UPROPERTY(BlueprintReadWrite, Category = "Game Settings")
	bool bHasPendingPlayerData = false;

	// ===== 存档便利接口（C 方案：多存档槽）=====
	// 直接挂在 GameInstance 上：蓝图里 `Get W Game Instance` 就能调用，无需手动拖子系统节点。
	// 内部全部转发给 UWGameSaveSubsystem，SlotIndex 语义一致（0=槽1，1=槽2，2=槽3）。
	UFUNCTION(BlueprintPure, Category = "Warrior|Save")
	UWGameSaveSubsystem* GetSaveSubsystem() const;

	UFUNCTION(BlueprintPure, Category = "Warrior|Save")
	bool HasSaveInSlot(int32 SlotIndex = 0) const;

	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	bool TryLoadGameInSlot(FWPlayerSaveData& OutData, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	void SaveGameToSlot(const FWPlayerSaveData& InData, int32 SlotIndex = 0);

	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	void DeleteSaveInSlot(int32 SlotIndex = 0);
};
