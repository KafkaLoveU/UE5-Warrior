// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayEffectTypes.h"
#include "Templates/SubclassOf.h"
#include "WProjectilePoolSubsystem.generated.h"

class AActor;

/** 对象池日志分类（供 Statics 入口和子系统内部共用）。 */
DECLARE_LOG_CATEGORY_EXTERN(LogWProjectilePool, Log, All);

/**
 * 投掷物对象池子系统（UWorldSubsystem）
 *
 * 为什么用 UWorldSubsystem 而不是 UGameInstanceSubsystem？
 *   - 投掷物只在某个关卡里存在；关卡切换时希望池一并被 GC，避免脏数据。
 *   - UWorldSubsystem 会在 World 销毁时自动 Deinitialize，刚好符合池的生命周期。
 *
 * 数据结构：
 *   TMap<TSubclassOf<AActor>, FActorPool> Pools;
 *     - 按投掷物精确子类分桶，每桶独立 Free/Active 列表。
 *     - 这样未来加"火球池"、"冰锥池"互不干扰。
 *
 * 增长策略（懒扩容）：
 *   - 首次 SpawnProjectileFromPool：池为空 → 创建一个对象（不预热）。
 *   - 池空了才扩容，每次 +1，避免一次性占用过多内存。
 *   - 设置 MaxPoolSize 软上限：超过则丢弃新对象（防止关卡死循环）。
 */
UCLASS()
class WARRIOR_API UWProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem

	/**
	 * 从池中取一个投掷物（蓝图入口在 WProjectilePoolStatics）。
	 * @param ProjectileClass   投掷物精确子类
	 * @param SpawnTransform    出现位置/朝向
	 * @param Instigator        伤害的发起者（用于 GAS spec 计算）
	 * @param DamageSpecHandle  已构造好的伤害 GE spec
	 * @return                  可用实例；池空了且超过 MaxPoolSize 时返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|ProjectilePool")
	AActor* AcquireProjectile(TSubclassOf<AActor> ProjectileClass,
	                          const FTransform& SpawnTransform,
	                          AActor* Instigator,
	                          const FGameplayEffectSpecHandle& DamageSpecHandle);

	/**
	 * 把投掷物归还到池中（替代 AActor::Destroy()）。
	 * 该方法是 Object 内部 + 蓝图都可调用的。
	 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|ProjectilePool")
	void ReleaseProjectile(AActor* Projectile);

	/** 配置：池容量上限。关卡中可动态调整。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Warrior|ProjectilePool")
	int32 MaxPoolSize = 32;

	/** 调试：打印池统计信息（活跃/空闲/总创建数）。 */
	UFUNCTION(BlueprintCallable, Category = "Warrior|ProjectilePool")
	void DumpPoolStats() const;

private:
	/** 单个类型桶：所有可用对象集合（双向链表，AddUnique 不需要，但 Remove 时 O(1)）。 */
	struct FActorPool
	{
		TArray<TWeakObjectPtr<AActor>> AllActors;   // 创建过的所有实例（用于关卡销毁时清场）
		TArray<TWeakObjectPtr<AActor>> FreeActors;  // 当前可用
	};

	/** 按子类分桶的池表。 */
	TMap<TSubclassOf<AActor>, FActorPool> Pools;

	/** 创建一个新实例（用 NewObject + 注册到 World + 调 OnAcquiredFromPool）。 */
	AActor* CreateNewPooledInstance(TSubclassOf<AActor> ProjectileClass,
	                                const FTransform& SpawnTransform,
	                                AActor* Instigator,
	                                const FGameplayEffectSpecHandle& DamageSpecHandle);

	/** 关卡销毁时调 OnRemovedFromPool 真正释放。 */
	void DestroyPoolContents();
};