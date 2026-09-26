// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WUserWidgetBase.h"
#include "Items/Inventory/WItemDefinition.h"
#include "WItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UWItemTooltip;
class UWItemDragDropOperation;

/**
 * 背包里单个物品格子的 UI 基类（Step 6/7）。
 * 蓝图子类（WBP_ItemSlot）只需在 Designer 里放一个 Image（变量名必须为 ItemIcon）
 * 和一个 Text（变量名必须为 ItemCount），其余填充逻辑都在 C++ 里完成。
 * 通过 SetItemData() 接收 FWInventoryItem，自动加载图标贴图并显示堆叠数量。
 * Step 9：右键格子触发使用/消耗（UseItem）。
 * Step 10：悬停格子显示物品说明 Tooltip（ItemTooltipClass）。
 * Step 11：左键拖起本格子、落到另一格子触发换位（SwapItems）。
 */
UCLASS()
class WARRIOR_API UWItemSlotWidget : public UWUserWidgetBase
{
	GENERATED_BODY()

public:
	// 由背包 UI（UWInventoryWidget::RefreshInventory）调用，传入要显示的物品及其所在槽位索引
	// 注：默认参数用字面量 -1（即 INDEX_NONE），UHT 不能解析 INDEX_NONE 宏。
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void SetItemData(const FWInventoryItem& InItem, int32 SlotIndex = -1);

protected:
	// 绑定到蓝图里同名控件（Designer 中控件变量名必须一致）
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> ItemIcon;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemCount;

	// 缓存当前物品（供蓝图扩展用）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	FWInventoryItem CachedItem;

	// 缓存当前格子在背包中的索引（Step 11 拖拽用：作为源/目标下标）
	UPROPERTY(BlueprintReadOnly, Category = "Inventory")
	int32 CachedSlotIndex = INDEX_NONE;

	// 悬停 Tooltip 控件类（Step 10），由 WBP_ItemSlot 的类默认值指定
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory")
	TSubclassOf<UWItemTooltip> ItemTooltipClass;

	// 右键格子 → 使用/消耗当前物品（Step 9）
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Step 11 拖拽：左键拖起时创建拖拽操作并带上源索引
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

	// Step 11 拖拽：允许在本格子上放下（基类签名用 FDragDropEvent，不是 FPointerEvent）
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InMouseEvent, UDragDropOperation* InOperation) override;

	// Step 11 拖拽：落到本格子时，用源索引与目标索引换位
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InMouseEvent, UDragDropOperation* InOperation) override;
};
