// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "WTypes/WEnumTypes.h"
#include "WSaveGame.generated.h"

/**
 * 一局完整进度（A 档：波数+难度；B 档起：再加角色成长属性与装备）。
 * 用结构体统一承载，以后加字段只改这里。
 */
USTRUCT(BlueprintType)
struct WARRIOR_API FWPlayerSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	int32 WaveCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	EWGameDifficulty Difficulty = EWGameDifficulty::Normal;

	// B 档：角色成长属性（捡石头永久提升后的值）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float MaxHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float MaxRage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float AttackPower = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float DefensePower = 0.f;

	// B 档：存档那一刻的真实当前值（精确续关，不再强制满血满怒气）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float CurrentHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	float CurrentRage = 0.f;

	// B 档：已装备武器标签（None 表示未装备）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
	FGameplayTag EquippedWeaponTag;
};

/**
 *
 */
UCLASS()
class WARRIOR_API UWSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 存档结构版本，读档时可据此做兼容处理
	UPROPERTY(BlueprintReadOnly)
	int32 SaveVersion = 0;

	// 整套进度数据（B 档起统一用这个结构）
	UPROPERTY(BlueprintReadOnly)
	FWPlayerSaveData PlayerData;
};
