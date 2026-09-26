// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Items/Pickups/WPickupBase.h"
#include "Items/Inventory/WItemDefinition.h"
#include "WItemPickup.generated.h"

class AWHeroCharacter;

/**
 * 场景中可拾取的物品 Actor：玩家与之 Overlap 时，把对应的 UWItemDefinition 加入背包。
 * 在蓝图子类（或本类默认值）的 Details 里设置 Item Definition 与 Quantity，
 * 并给 Pickup Mesh 指定一个可见的 Static Mesh。
 */
UCLASS(Blueprintable)
class WARRIOR_API AWItemPickup : public AWPickupBase
{
	GENERATED_BODY()

public:
	AWItemPickup();

	// 要加入背包的物品定义（Details 里指派，例如 DA_HealthPotion / DA_Stone）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup")
	TSoftObjectPtr<UWItemDefinition> ItemDefinition;

	// 每次拾取的数量
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pickup")
	int32 Quantity = 1;

	// 可见的物品模型（蓝图 Details 里指定 Static Mesh）
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	TObjectPtr<UStaticMeshComponent> PickupMesh;

protected:
	virtual void OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

	// 成功拾取后触发，蓝图可播放特效/音效
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Item Picked Up"))
	void BP_OnItemPickedUp();

	bool bPickedUp = false;
};
