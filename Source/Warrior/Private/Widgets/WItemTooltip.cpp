// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/WItemTooltip.h"
#include "Components/TextBlock.h"

void UWItemTooltip::SetTooltipData(const TSoftObjectPtr<UWItemDefinition>& InItemDef, int32 InStackCount)
{
	const UWItemDefinition* Def = InItemDef.LoadSynchronous();
	if (!Def)
	{
		return;
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(Def->DisplayName);
	}

	if (ItemDescText)
	{
		ItemDescText->SetText(Def->Description);
	}

	if (ItemTypeText)
	{
		// 取枚举的 UMETA(DisplayName) 中文名（消耗品 / 材料 / 装备 ...）
		if (const UEnum* Enum = StaticEnum<EWItemType>())
		{
			ItemTypeText->SetText(Enum->GetDisplayNameTextByValue(static_cast<int64>(Def->ItemType)));
		}
	}

	if (StackText)
	{
		StackText->SetText(FText::Format(
			NSLOCTEXT("ItemTooltip", "StackFmt", "数量 {0} / {1}"),
			FText::AsNumber(InStackCount),
			FText::AsNumber(Def->MaxStack)));
	}
}
