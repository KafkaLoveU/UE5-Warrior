// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "WProjectilePoolStatics.generated.h"

class AActor;
class UObject;

/**
 * 蓝图侧的对象池入口。
 *
 * 为什么需要这个静态库？
 *   - 蓝图节点不能直接拿到 UWorldSubsystem（没有 Get节点）。
 *   - 蓝图节点 = 静态方法 + WorldContext 参数引擎会自动注入。
 *   - 这样蓝图只要传 WorldContextObject 就能取到对应 World 的池。
 */
UCLASS()
class WARRIOR_API UWProjectilePoolStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 从对象池中获取一个投掷物（蓝图节点）。
	 * 用法：替换原 "SpawnActor from Class"，返回的 Actor 直接传给后续逻辑。
	 * 引脚刻意设计成 Location / Rotation 分开，和 SpawnActor 一致，方便直接换节点。
	 *
	 * @param WorldContextObject  世界内任意 UObject（蓝图自动传入 Self）
	 * @param ProjectileClass     投掷物精确子类
	 * @param Location            出现位置
	 * @param Rotation            出现朝向
	 * @param Instigator          发起者
	 * @param DamageSpecHandle    已构造好的伤害 spec
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|ProjectilePool",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Spawn Projectile From Pool"))
	static AActor* SpawnProjectileFromPool(UObject* WorldContextObject,
	                                        TSubclassOf<AActor> ProjectileClass,
	                                        FVector Location,
	                                        FRotator Rotation,
	                                        AActor* Instigator,
	                                        const FGameplayEffectSpecHandle& DamageSpecHandle);

	/** 把投掷物归还到池（蓝图节点）。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|ProjectilePool",
		meta = (WorldContext = "WorldContextObject", DisplayName = "Return Projectile To Pool"))
	static void ReturnProjectileToPool(UObject* WorldContextObject, AActor* Projectile);
};