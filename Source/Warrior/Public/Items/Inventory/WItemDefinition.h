// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/AssetManager.h"
#include "GameplayTagContainer.h"
#include "WItemDefinition.generated.h"

class UTexture2D;
class UGameplayEffect;
class UWItemDefinition;

// 物品大类。决定它在背包里的归类和右键能做什么。
UENUM(BlueprintType)
enum class EWItemType : uint8
{
	Consumable	UMETA(DisplayName = "消耗品"),
	Material	UMETA(DisplayName = "材料"),
	Equipment	UMETA(DisplayName = "装备"),
	Quest		UMETA(DisplayName = "任务道具"),
	Misc		UMETA(DisplayName = "杂项")
};

// 背包里的一个物品格子：物品定义的软对象引用 + 堆叠数量。
// 用软引用（TSoftObjectPtr）而非裸指针/硬引用，避免序列化、以后网络复制时持有无效对象，
// 也更省内存（没用到的物品定义不会被强制加载）。
USTRUCT(BlueprintType)
struct WARRIOR_API FWInventoryItem
{
	GENERATED_BODY()

	// 物品定义的软对象引用（指向 Content 里建的 UWItemDefinition 数据资产，如 DA_Stone）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TSoftObjectPtr<UWItemDefinition> ItemDefinition;

	// 当前堆叠数量
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 StackCount = 0;

	// 是否为有效物品（有定义且数量 > 0）
	bool bIsValid() const { return ItemDefinition.IsValid() && StackCount > 0; }
};

/**
 * 单个物品的定义（数据驱动）。在编辑器 Content 浏览器里右键 -> 杂项 -> Data Asset -> 选 UWItemDefinition 创建。
 * 只描述"这个物品是什么"，不含运行时状态；运行时状态由 FWInventoryItem 承载。
 */
UCLASS(BlueprintType)
class WARRIOR_API UWItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// 物品唯一标识（GameplayTag）。以后按 ID 查找、存档都靠它，而不是靠资产名。
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FGameplayTag ItemID;

	// 显示名（HUD/Tooltip 用）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	// 描述（Tooltip 用）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", meta = (MultiLine = "true"))
	FText Description;

	// 图标（软引用，用到才加载）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	// 物品大类
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EWItemType ItemType = EWItemType::Misc;

	// 单格最大堆叠数
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	int32 MaxStack = 99;

	// 是否可被消耗（右键"使用"）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	bool bIsConsumable = false;

	// 消耗时施加的 GameplayEffect（Step 9 接入使用逻辑，这里先留字段）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TSubclassOf<UGameplayEffect> ConsumeEffect;

	// 进入 Asset Registry，便于以后按 PrimaryAssetId 加载/查询。
	// 资产类型固定为 "Item"，ID 用资产名（与文件名一致即可被 Asset Manager 发现）。
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
};
