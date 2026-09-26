// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WUserWidgetBase.h"
#include "Items/Inventory/WItemDefinition.h"
#include "WItemTooltip.generated.h"

class UTextBlock;

/**
 * 背包物品悬停说明浮窗（Step 10）。
 * 蓝图子类（WBP_ItemTooltip）在 Designer 里放 4 个 Text，变量名必须匹配：
 *   ItemNameText  - 物品显示名（DisplayName）
 *   ItemDescText  - 物品描述（Description，可多行）
 *   ItemTypeText  - 物品大类（EWItemType 的中文显示名：消耗品/材料/装备...）
 *   StackText     - 堆叠数量（x / MaxStack）
 * 由 UWItemSlotWidget::SetItemData 在运行时创建并填充数据。
 */
UCLASS()
class WARRIOR_API UWItemTooltip : public UWUserWidgetBase
{
	GENERATED_BODY()

public:
	// 由物品格子调用，传入要说明的物品定义与当前堆叠数
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetTooltipData(const TSoftObjectPtr<UWItemDefinition>& InItemDef, int32 InStackCount);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemDescText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemTypeText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StackText;
};
