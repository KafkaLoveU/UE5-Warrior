// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGame/WSaveGame.h"
#include "WGameSaveSubsystem.generated.h"

/**
 * 存档子系统（挂在 GameInstance 上）。
 * 统一管理游戏进度存档，避免每次保存都新建对象导致其它字段被清空。
 */
UCLASS()
class WARRIOR_API UWGameSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** 存一局完整进度（波数 + 难度 + 角色成长属性 + 装备）。内部走「先读旧档 → 改字段 → 写回」。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	void SaveGame(const FWPlayerSaveData& InData);

	/** 读取存档到 OutData。无存档或读取出错返回 false。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	bool TryLoadGame(FWPlayerSaveData& OutData);

	/** 是否存在存档。 */
	UFUNCTION(BlueprintPure, Category = "Warrior|Save")
	bool HasSave() const;

	/** 删除存档（慎用）。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	void DeleteSave();

private:
	FString GetSlotName() const;

	static constexpr int32 CurrentSaveVersion = 1;
};
