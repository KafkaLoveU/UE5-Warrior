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

	// 可用存档槽数量（3 个：槽 1 / 2 / 3）
	static constexpr int32 MaxSaveSlots = 3;

	/** 槽位索引(0/1/2) → 磁盘存档名。越界自动夹到 [0, MaxSaveSlots-1]。 */
	static FString GetSlotName(int32 SlotIndex = 0);

	/** 存一局完整进度（波数 + 难度 + 角色成长属性 + 装备）。内部走「先读旧档 → 改字段 → 写回」。
	 *  SlotIndex 指定写入哪个槽（默认 0 = 槽 1）。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	void SaveGame(const FWPlayerSaveData& InData, int32 SlotIndex = 0);

	/** 读取指定槽存档到 OutData。无存档或读取出错返回 false。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	bool TryLoadGame(FWPlayerSaveData& OutData, int32 SlotIndex = 0);

	/** 指定槽是否存在存档。 */
	UFUNCTION(BlueprintPure, Category = "Warrior|Save")
	bool HasSave(int32 SlotIndex = 0) const;

	/** 删除指定槽存档（慎用）。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|Save")
	void DeleteSave(int32 SlotIndex = 0);

private:
	static constexpr int32 CurrentSaveVersion = 1;
};
