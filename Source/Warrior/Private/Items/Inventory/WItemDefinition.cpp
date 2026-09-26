// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Inventory/WItemDefinition.h"

FPrimaryAssetId UWItemDefinition::GetPrimaryAssetId() const
{
	// 资产类型固定为 "Item"，ID 用资产名（与文件名一致即可被 Asset Manager 发现）。
	return FPrimaryAssetId("Item", GetFName());
}
