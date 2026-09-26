// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Items/Inventory/WItemDefinition.h"
#include "SaveGame/WSaveGame.h"
#include "WInventoryComponent.generated.h"

// 背包整体变化时广播（UI 据此全量刷新）。单机即可用；上网络时配合属性复制回调。
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChangedSignature);

/**
 * 背包组件：挂在 AWHeroCharacter 上（v1 单机方案；以后上网络可整体挪到 PlayerState）。
 * 数据用 TArray<FWInventoryItem>（内部是软对象引用），不持有裸指针，便于序列化与复制。
 * 容量 / 堆叠 / 溢出逻辑全部在 C++ 内，UI 只监听 OnInventoryChanged 刷新。
 */
UCLASS(ClassGroup = (Inventory), Blueprintable, meta = (BlueprintSpawnableComponent))
class WARRIOR_API UWInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWInventoryComponent();

	// 背包容量（格子数）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inventory")
	int32 Capacity = 30;

	// 加入物品，返回实际加入数量（背包满则溢出部分不加入）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 AddItem(const TSoftObjectPtr<UWItemDefinition>& ItemDef, int32 Count = 1);

	// 移除物品，返回实际移除数量（不足则返回现有全部）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 RemoveItem(const TSoftObjectPtr<UWItemDefinition>& ItemDef, int32 Count = 1);

	// 查询某物品总数量
	UFUNCTION(BlueprintPure, Category = "Inventory")
	int32 GetItemCount(const TSoftObjectPtr<UWItemDefinition>& ItemDef) const;

	// 使用/消耗物品（Step 9）。仅当物品 bIsConsumable 且配置了 ConsumeEffect 时生效：
	// 对持有者施加对应 GameplayEffect，并减少 Count 个堆叠。返回是否成功使用。
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool UseItem(const TSoftObjectPtr<UWItemDefinition>& ItemDef, int32 Count = 1);

	// 交换/调换两个槽位的物品（Step 11 拖拽排序）。索引越界或相同则忽略。
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SwapItems(int32 FromIndex, int32 ToIndex);

	// 把当前背包导出到存档数据（Step 12，按格子顺序原样保存）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ExportToSaveData(FWPlayerSaveData& OutData);

	// 从存档数据恢复背包（Step 12：清空后逐格导入，保持存档时的格子布局）
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ImportFromSaveData(const FWPlayerSaveData& InData);

	// 只读访问整个背包（供 UI 遍历显示）
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const TArray<FWInventoryItem>& GetItems() const { return Items; }

	// 背包变化（增 / 删 / 堆叠变化）时广播，UI 绑定它刷新
	UPROPERTY(BlueprintAssignable, Category = "Inventory")
	FOnInventoryChangedSignature OnInventoryChanged;

protected:
	UPROPERTY()
	TArray<FWInventoryItem> Items;

	// 取某物品单格最大堆叠（按需同步加载定义资产；未定义时回落 99）
	int32 GetMaxStackFor(const TSoftObjectPtr<UWItemDefinition>& ItemDef) const;
};
