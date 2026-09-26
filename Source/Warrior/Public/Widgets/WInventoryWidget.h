// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/WUserWidgetBase.h"
#include "Components/WrapBox.h"
#include "Widgets/WItemSlotWidget.h"
#include "WInventoryWidget.generated.h"

class UHeroUIComponent;
class UWInventoryComponent;

/**
 * 背包 UI 基类（Step 4 起）。
 * - 显隐切换：由英雄在创建 Widget 后调用 BindToHeroUI() 订阅 UHeroUIComponent::OnToggleInventory，
 *   收到事件就把自身 Visibility 在 Collapsed(关) 与 Visible(开) 之间翻转。
 * - 物品显示（Step 6/7）：订阅英雄 UWInventoryComponent::OnInventoryChanged，数据变化时
 *   调用 RefreshInventory() 重建格子。格子模板由蓝图在 Class Defaults 指定（ItemSlotWidgetClass），
 *   格子容器由蓝图 Designer 里命名为 SlotContainer 的 Wrap Box 提供（BindWidget）。
 * 本类保持精简，具体排版交给蓝图子类 WBP_Inventory / WBP_ItemSlot。
 */
UCLASS()
class WARRIOR_API UWInventoryWidget : public UWUserWidgetBase
{
	GENERATED_BODY()

public:
	// 由英雄（WHeroCharacter::BeginPlay）在 Widget 创建后调用，把"切换背包"事件接到显隐逻辑上。
	UFUNCTION(BlueprintCallable)
	void BindToHeroUI(UHeroUIComponent* HeroUI);

	// 背包数据变化时（增/删/堆叠）由 UWInventoryComponent::OnInventoryChanged 触发，重建格子。
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void RefreshInventory();

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void OnToggleInventoryReceived();

	UFUNCTION()
	void OnInventoryChangedReceived();

	// 绑定到蓝图里同名容器（Designer 中放一个 Wrap Box，变量名必须为 SlotContainer）
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UWrapBox> SlotContainer;

	// 蓝图 Class Defaults 里设成 WBP_ItemSlot（继承 UWItemSlotWidget 的控件蓝图）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UWItemSlotWidget> ItemSlotWidgetClass;

	// 缓存英雄身上挂的背包组件
	UPROPERTY()
	TWeakObjectPtr<UWInventoryComponent> CachedInventory;
};
