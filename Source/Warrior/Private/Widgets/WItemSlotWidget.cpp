// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/WItemSlotWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Characters/WHeroCharacter.h"
#include "Components/Inventory/WInventoryComponent.h"
#include "Widgets/WItemTooltip.h"
#include "Widgets/WItemDragDropOperation.h"
#include "InputCoreTypes.h"

void UWItemSlotWidget::SetItemData(const FWInventoryItem& InItem, int32 SlotIndex)
{
	CachedItem = InItem;
	CachedSlotIndex = SlotIndex;

	// 先清空，避免复用时残留旧图标/数字
	if (ItemIcon)
	{
		ItemIcon->SetBrushFromTexture(nullptr);
	}
	if (ItemCount)
	{
		ItemCount->SetText(FText::GetEmpty());
	}

	// ItemDefinition 是软引用，未加载时 LoadSynchronous() 同步加载一次（图标贴图很小，开销可接受）
	if (const UWItemDefinition* Def = InItem.ItemDefinition.LoadSynchronous())
	{
		if (ItemIcon)
		{
			if (UTexture2D* Tex = Def->Icon.LoadSynchronous())
			{
				ItemIcon->SetBrushFromTexture(Tex);
			}
		}

		if (ItemCount)
		{
			// 数量 > 1 才显示数字，单格单件不显示
			if (InItem.StackCount > 1)
			{
				ItemCount->SetText(FText::AsNumber(InItem.StackCount));
			}
			else
			{
				ItemCount->SetText(FText::GetEmpty());
			}
		}
	}

	// Step 10：悬停 Tooltip —— 创建说明浮窗并挂到本格子上（UMG 悬停自动显示）
	if (ItemTooltipClass)
	{
		if (UWItemTooltip* Tooltip = CreateWidget<UWItemTooltip>(this, ItemTooltipClass))
		{
			Tooltip->SetTooltipData(InItem.ItemDefinition, InItem.StackCount);
			SetToolTip(Tooltip);
		}
	}
}

FReply UWItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 右键：使用/消耗当前物品
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (AWHeroCharacter* Hero = Cast<AWHeroCharacter>(GetOwningPlayerPawn()))
		{
			if (UWInventoryComponent* Inv = Hero->GetInventoryComponent())
			{
				Inv->UseItem(CachedItem.ItemDefinition, 1);
			}
		}
		return FReply::Handled();
	}

	// 左键：开始拖拽（Step 11）。引擎检测到鼠标移动超过阈值后自动调用 NativeOnDragDetected
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UWItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	// 创建拖拽操作，带上本格子的源索引，拖拽时显示本格子外观
	UWItemDragDropOperation* Op = NewObject<UWItemDragDropOperation>(this);
	Op->SourceSlotIndex = CachedSlotIndex;
	Op->DefaultDragVisual = this;
	Op->Pivot = EDragPivot::MouseDown;
	OutOperation = Op;
}

bool UWItemSlotWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InMouseEvent, UDragDropOperation* InOperation)
{
	// 允许把物品放到本格子上（返回 false 则引擎不会调用 NativeOnDrop）
	return true;
}

bool UWItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InMouseEvent, UDragDropOperation* InOperation)
{
	if (UWItemDragDropOperation* ItemOp = Cast<UWItemDragDropOperation>(InOperation))
	{
		if (AWHeroCharacter* Hero = Cast<AWHeroCharacter>(GetOwningPlayerPawn()))
		{
			if (UWInventoryComponent* Inv = Hero->GetInventoryComponent())
			{
				Inv->SwapItems(ItemOp->SourceSlotIndex, CachedSlotIndex);
				return true;
			}
		}
	}
	return false;
}
