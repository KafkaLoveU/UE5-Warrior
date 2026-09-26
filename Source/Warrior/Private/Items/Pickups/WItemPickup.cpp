// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/Pickups/WItemPickup.h"
#include "Characters/WHeroCharacter.h"
#include "Components/Inventory/WInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AWItemPickup::AWItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>("PickupMesh");
	// TObjectPtr<USphereComponent> 不会隐式转换为 USceneComponent*，需显式 .Get() 取裸指针；
	// 且本 cpp 必须 include "Components/SphereComponent.h" 让 USphereComponent 完整可见，否则向上转型失败
	PickupMesh->SetupAttachment(PickupCollisionSphere.Get());
	// 碰撞检测交给基类球体，Mesh 本身不参与碰撞
	PickupMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 基类 AWPickupBase 只建了碰撞球、没配碰撞：这里补全，否则 OnComponentBeginOverlap 永不触发、
	// 角色会直接穿过去。球设为 QueryOnly + 生成重叠事件 + 仅对 Pawn 通道 Overlap（其余忽略），
	// 玩家走上去即触发拾取回调（石头不走这条是因为它用 GAS 能力主动扫描，所以基类一直没配）。
	if (USphereComponent* Sphere = PickupCollisionSphere.Get())
	{
		Sphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Sphere->SetGenerateOverlapEvents(true);
		Sphere->SetCollisionResponseToAllChannels(ECR_Ignore);
		Sphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
}

void AWItemPickup::OnCollisionSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[ItemPickup] Overlap begin with %s"), OtherActor ? *OtherActor->GetName() : TEXT("null"));

	if (bPickedUp)
	{
		return;
	}

	// 软引用用 IsNull() 判断"路径有没有在编辑器里指派"，不能用 IsValid()——
	// 后者要求资产已加载到内存，运行时未加载会返回 false 而误跳过拾取。
	if (ItemDefinition.IsNull())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ItemPickup] ItemDefinition not set, skip"));
		return;
	}

	if (AWHeroCharacter* OverlappedHero = Cast<AWHeroCharacter>(OtherActor))
	{
		if (UWInventoryComponent* Inventory = OverlappedHero->GetInventoryComponent())
		{
			const int32 AddedCount = Inventory->AddItem(ItemDefinition, Quantity);
			UE_LOG(LogTemp, Warning, TEXT("[ItemPickup] AddItem returned %d"), AddedCount);

			if (AddedCount > 0)
			{
				bPickedUp = true;

				// 关掉碰撞并隐藏自身，避免同一物品被重复拾取
				SetActorEnableCollision(false);
				SetActorHiddenInGame(true);

				BP_OnItemPickedUp();
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[ItemPickup] Hero has no InventoryComponent"));
		}
	}
}
