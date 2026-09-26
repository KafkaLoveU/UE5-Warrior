// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "WItemDragDropOperation.generated.h"

/**
 * 拖拽操作（Step 11）：携带"源格子索引"。
 * 从源格子拖到目标格子时，目标格子在 NativeOnDrop 里读 SourceSlotIndex，
 * 调用背包的 SwapItems(源, 目标) 完成两个槽位换位。
 */
UCLASS()
class WARRIOR_API UWItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// 被拖拽物品所在的源槽位索引
	UPROPERTY(BlueprintReadWrite, Category = "Inventory")
	int32 SourceSlotIndex = INDEX_NONE;
};
