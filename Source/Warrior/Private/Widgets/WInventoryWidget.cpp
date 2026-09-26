// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/WInventoryWidget.h"
#include "Components/UI/HeroUIComponent.h"
#include "Components/Inventory/WInventoryComponent.h"
#include "Items/Inventory/WItemDefinition.h"
#include "Characters/WHeroCharacter.h"
#include "Components/WrapBox.h"
#include "GameFramework/PlayerController.h"

void UWInventoryWidget::BindToHeroUI(UHeroUIComponent* HeroUI)
{
	if (HeroUI)
	{
		// AddUniqueDynamic 防止重复绑定（如 Widget 被多次初始化）
		HeroUI->OnToggleInventory.AddUniqueDynamic(this, &UWInventoryWidget::OnToggleInventoryReceived);
	}
}

void UWInventoryWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 从拥有的玩家 pawn 取到英雄，再取背包组件，订阅其变化委托并首次刷新
	if (AWHeroCharacter* Hero = Cast<AWHeroCharacter>(GetOwningPlayerPawn()))
	{
		if (UWInventoryComponent* Inv = Hero->GetInventoryComponent())
		{
			CachedInventory = Inv;
			Inv->OnInventoryChanged.AddUniqueDynamic(this, &UWInventoryWidget::OnInventoryChangedReceived);
			RefreshInventory();
		}
	}
}

void UWInventoryWidget::OnInventoryChangedReceived()
{
	RefreshInventory();
}

void UWInventoryWidget::RefreshInventory()
{
	if (!SlotContainer)
	{
		return;
	}

	SlotContainer->ClearChildren();

	if (!CachedInventory.IsValid() || !ItemSlotWidgetClass)
	{
		return;
	}

	// 用索引遍历，把每个格子的槽位下标传给 SetItemData（Step 11 拖拽排序要用索引定位源/目标）
	const TArray<FWInventoryItem>& Items = CachedInventory->GetItems();
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (UWItemSlotWidget* SlotWidget = CreateWidget<UWItemSlotWidget>(this, ItemSlotWidgetClass))
		{
			SlotWidget->SetItemData(Items[i], i);
			SlotContainer->AddChild(SlotWidget);
		}
	}
}

void UWInventoryWidget::OnToggleInventoryReceived()
{
	// 当前为 Visible（打开）则关闭，否则打开。本 Widget 仅用 Collapsed/Visible 两态切换。
	const bool bWasOpen = (GetVisibility() == ESlateVisibility::Visible);

	SetVisibility(bWasOpen ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);

	// 打开时显示鼠标光标并切换为 GameAndUI，让格子能接收右键/拖拽；关闭时还原 GameOnly。
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (bWasOpen)
		{
			// 原本开着 -> 现在关闭
			PC->bShowMouseCursor = false;
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
		}
		else
		{
			// 原本关着 -> 现在打开
			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(this->TakeWidget());
			PC->SetInputMode(InputMode);
		}
	}
}
