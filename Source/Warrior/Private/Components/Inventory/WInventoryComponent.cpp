// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Inventory/WInventoryComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

UWInventoryComponent::UWInventoryComponent()
{
	// 背包不需要每帧 Tick
	PrimaryComponentTick.bCanEverTick = false;

	// 单机：默认不复制。上网络时改为 SetIsReplicatedByDefault(true) 并在 GetLifetimeReplicatedProps 加 DOREPLIFETIME，
	// 背包逻辑本身无需改动。
	SetIsReplicatedByDefault(false);
}

int32 UWInventoryComponent::GetMaxStackFor(const TSoftObjectPtr<UWItemDefinition>& ItemDef) const
{
	// TSoftObjectPtr 软引用的是「对象实例」（数据资产），LoadSynchronous() 直接返回 UWItemDefinition*。
	// 已加载时开销极小；未加载才同步加载一次。
	if (const UWItemDefinition* Def = ItemDef.LoadSynchronous())
	{
		return Def->MaxStack;
	}
	return 99;
}

int32 UWInventoryComponent::AddItem(const TSoftObjectPtr<UWItemDefinition>& ItemDef, int32 Count)
{
	// 软引用用 IsNull() 判断"路径是否已指派"；不能用 IsValid()（要求已加载，未加载会误判为无效）
	if (ItemDef.IsNull() || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;
	const int32 MaxStack = GetMaxStackFor(ItemDef);

	// 1) 先合并到已有的同物、未满栈格子
	for (FWInventoryItem& It : Items)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (It.ItemDefinition == ItemDef && It.StackCount < MaxStack)
		{
			const int32 Space = MaxStack - It.StackCount;
			const int32 Add = FMath::Min(Space, Remaining);
			It.StackCount += Add;
			Remaining -= Add;
		}
	}

	// 2) 还有剩余且未满容量，开新格子
	while (Remaining > 0 && Items.Num() < Capacity)
	{
		const int32 Add = FMath::Min(MaxStack, Remaining);
		FWInventoryItem NewItem;
		NewItem.ItemDefinition = ItemDef;
		NewItem.StackCount = Add;
		Items.Add(NewItem);
		Remaining -= Add;
	}

	const int32 Added = Count - Remaining;
	if (Added > 0)
	{
		OnInventoryChanged.Broadcast();
	}
	return Added;
}

int32 UWInventoryComponent::RemoveItem(const TSoftObjectPtr<UWItemDefinition>& ItemDef, int32 Count)
{
	if (ItemDef.IsNull() || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;
	// 从后往前移除，方便 RemoveAt 不破坏索引
	for (int32 i = Items.Num() - 1; i >= 0; --i)
	{
		if (Remaining <= 0)
		{
			break;
		}
		FWInventoryItem& It = Items[i];
		if (It.ItemDefinition == ItemDef)
		{
			const int32 Take = FMath::Min(It.StackCount, Remaining);
			It.StackCount -= Take;
			Remaining -= Take;
			if (It.StackCount <= 0)
			{
				Items.RemoveAt(i);
			}
		}
	}

	const int32 Removed = Count - Remaining;
	if (Removed > 0)
	{
		OnInventoryChanged.Broadcast();
	}
	return Removed;
}

int32 UWInventoryComponent::GetItemCount(const TSoftObjectPtr<UWItemDefinition>& ItemDef) const
{
	if (ItemDef.IsNull())
	{
		return 0;
	}
	int32 Total = 0;
	for (const FWInventoryItem& It : Items)
	{
		if (It.ItemDefinition == ItemDef)
		{
			Total += It.StackCount;
		}
	}
	return Total;
}

bool UWInventoryComponent::UseItem(const TSoftObjectPtr<UWItemDefinition>& ItemDef, int32 Count)
{
	if (ItemDef.IsNull() || Count <= 0)
	{
		return false;
	}

	// 背包里数量不足，无法使用
	if (GetItemCount(ItemDef) < Count)
	{
		return false;
	}

	const UWItemDefinition* Def = ItemDef.LoadSynchronous();
	if (!Def || !Def->bIsConsumable || !Def->ConsumeEffect)
	{
		return false;
	}

	// 通过持有者（英雄）的 ASC 施加消耗效果
	bool bApplied = false;
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
		{
			const UGameplayEffect* EffectCDO = Def->ConsumeEffect.GetDefaultObject();
			ASC->ApplyGameplayEffectToSelf(EffectCDO, 1.f, ASC->MakeEffectContext());
			bApplied = true;
		}
	}

	if (bApplied)
	{
		RemoveItem(ItemDef, Count);
		return true;
	}

	return false;
}

void UWInventoryComponent::SwapItems(int32 FromIndex, int32 ToIndex)
{
	// 越界或原地交换则忽略
	if (!Items.IsValidIndex(FromIndex) || !Items.IsValidIndex(ToIndex) || FromIndex == ToIndex)
	{
		return;
	}

	// 直接交换两个槽位的内容（Step 11 拖拽排序）。同物合并可后续增强，先交付换位。
	const FWInventoryItem Tmp = Items[FromIndex];
	Items[FromIndex] = Items[ToIndex];
	Items[ToIndex] = Tmp;

	OnInventoryChanged.Broadcast();
}

void UWInventoryComponent::ExportToSaveData(FWPlayerSaveData& OutData)
{
	// 按格子顺序原样导出（FWInventoryItem 内部是软引用 + 数量，可直接序列化）
	OutData.InventoryItems = Items;
}

void UWInventoryComponent::ImportFromSaveData(const FWPlayerSaveData& InData)
{
	Items.Reset();

	// 逐格导入：跳过无效条目，堆叠数夹到单格上限，且不超过背包容量。
	// 不做跨格合并，保持存档时的格子布局（读档后格子顺序与存档时一致）。
	for (const FWInventoryItem& Saved : InData.InventoryItems)
	{
		if (Items.Num() >= Capacity)
		{
			break;
		}
		if (Saved.ItemDefinition.IsNull() || Saved.StackCount <= 0)
		{
			continue;
		}

		FWInventoryItem NewItem;
		NewItem.ItemDefinition = Saved.ItemDefinition;
		NewItem.StackCount = FMath::Min(Saved.StackCount, GetMaxStackFor(Saved.ItemDefinition));
		Items.Add(NewItem);
	}

	OnInventoryChanged.Broadcast();
}
