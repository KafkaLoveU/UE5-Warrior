// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#include "Subsystems/WProjectilePoolSubsystem.h"

#include "Engine/World.h"
#include "Interfaces/ProjectilePoolableInterface.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY(LogWProjectilePool);

bool UWProjectilePoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 只在有 World 且 World 有效时创建（过滤掉编辑器预览 World 等特殊情况）。
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE;
	}
	return false;
}

void UWProjectilePoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogWProjectilePool, Log, TEXT("[ProjectilePool] Initialized. MaxPoolSize=%d"), MaxPoolSize);
}

void UWProjectilePoolSubsystem::Deinitialize()
{
	DestroyPoolContents();
	Super::Deinitialize();
}

AActor* UWProjectilePoolSubsystem::AcquireProjectile(TSubclassOf<AActor> ProjectileClass,
                                                     const FTransform& SpawnTransform,
                                                     AActor* Instigator,
                                                     const FGameplayEffectSpecHandle& DamageSpecHandle)
{
	if (!ProjectileClass)
	{
		UE_LOG(LogWProjectilePool, Warning, TEXT("[ProjectilePool] AcquireProjectile called with null class."));
		return nullptr;
	}

	FActorPool& Pool = Pools.FindOrAdd(ProjectileClass);

	// 1) 优先从 Free 队列复用（注意清理已 GC 的弱引用）。
	while (Pool.FreeActors.Num() > 0)
	{
		TWeakObjectPtr<AActor> WeakActor = Pool.FreeActors.Pop(false /* bAllowShrinking */);
		if (AActor* Actor = WeakActor.Get())
		{
			// 调用接口：子类会在这里重置组件、Transform、伤害句柄等。
			if (Actor->Implements<UProjectilePoolableInterface>())
			{
				IProjectilePoolableInterface::Execute_OnAcquiredFromPool(Actor, SpawnTransform, DamageSpecHandle);
			}
			Actor->SetInstigator(Cast<APawn>(Instigator));
			UE_LOG(LogWProjectilePool, Display, TEXT("[ProjectilePool] Reused pooled instance of '%s' from free list (Free left: %d)."),
				*ProjectileClass->GetName(), Pool.FreeActors.Num());
			return Actor;
		}
		// 如果 WeakPtr 已失效（GC 走了一半），循环下一个
	}

	// 2) Free 队列为空 → 池未满则新建；已满则复用最旧活跃实例（不丢投掷物、不无限扩容）。
	if (Pool.AllActors.Num() >= MaxPoolSize)
	{
		if (AActor* Recycled = RecycleOldestActiveInstance(Pool, SpawnTransform, DamageSpecHandle))
		{
			UE_LOG(LogWProjectilePool, Warning,
				TEXT("[ProjectilePool] Pool for class '%s' is full (MaxPoolSize=%d). Reusing oldest active instance instead of dropping."),
				*ProjectileClass->GetName(), MaxPoolSize);
			Recycled->SetInstigator(Cast<APawn>(Instigator));
			return Recycled;
		}
		// 回收不到（全为失效弱引用）→ 兜底新建，避免直接丢弃投掷物。
		UE_LOG(LogWProjectilePool, Warning,
			TEXT("[ProjectilePool] Pool for class '%s' is full and no recyclable instance found. Creating overflow instance."),
			*ProjectileClass->GetName());
	}

	return CreateNewPooledInstance(ProjectileClass, SpawnTransform, Instigator, DamageSpecHandle);
}

AActor* UWProjectilePoolSubsystem::CreateNewPooledInstance(TSubclassOf<AActor> ProjectileClass,
                                                           const FTransform& SpawnTransform,
                                                           AActor* Instigator,
                                                           const FGameplayEffectSpecHandle& DamageSpecHandle)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// SpawnActor 默认走完整 BeginPlay。我们紧接着会立刻 Deactivate 再 Activate 来正确初始化池状态。
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.Owner = Instigator ? Instigator->GetInstigator() : nullptr;
	SpawnParams.Instigator = Cast<APawn>(Instigator);

	AActor* NewActor = World->SpawnActor<AActor>(ProjectileClass, SpawnTransform, SpawnParams);
	if (!NewActor)
	{
		UE_LOG(LogWProjectilePool, Error, TEXT("[ProjectilePool] SpawnActor failed for class '%s'."), *ProjectileClass->GetName());
		return nullptr;
	}

	// 通知子类完成池初始化（设置伤害 spec、重启 Niagara、启动寿命定时器）
	if (NewActor->Implements<UProjectilePoolableInterface>())
	{
		IProjectilePoolableInterface::Execute_OnAcquiredFromPool(NewActor, SpawnTransform, DamageSpecHandle);
	}

	// 记账
	FActorPool& Pool = Pools.FindOrAdd(ProjectileClass);
	Pool.AllActors.Add(NewActor);

	UE_LOG(LogWProjectilePool, Verbose, TEXT("[ProjectilePool] Created new pooled instance of '%s'. Total now: %d"),
		*ProjectileClass->GetName(), Pool.AllActors.Num());

	return NewActor;
}

AActor* UWProjectilePoolSubsystem::RecycleOldestActiveInstance(FActorPool& Pool,
                                                              const FTransform& SpawnTransform,
                                                              const FGameplayEffectSpecHandle& DamageSpecHandle)
{
	// AllActors 按创建顺序保存，从最旧（下标 0）往新遍历：
	//  - 失效弱引用：顺手清理，避免死指针污染容量计数；
	//  - 处于 Free（空闲）状态的：跳过（本分支进入时通常已空，这里仅作保险）；
	//  - 第一个仍然有效的活跃实例：复位其池状态后作为新发射体复用。
	for (int32 i = 0; i < Pool.AllActors.Num(); ++i)
	{
		TWeakObjectPtr<AActor> WeakActor = Pool.AllActors[i];
		AActor* Actor = WeakActor.Get();
		if (!Actor)
		{
			Pool.AllActors.RemoveAt(i);
			--i; // 抵消自减，继续遍历
			continue;
		}

		if (Pool.FreeActors.Contains(WeakActor))
		{
			continue;
		}

		// 找到一个活跃实例：复位并复用（OnAcquiredFromPool 内部已清旧定时器/速度/Overlap 状态）。
		if (Actor->Implements<UProjectilePoolableInterface>())
		{
			IProjectilePoolableInterface::Execute_OnAcquiredFromPool(Actor, SpawnTransform, DamageSpecHandle);
		}
		return Actor;
	}
	return nullptr;
}

void UWProjectilePoolSubsystem::ReleaseProjectile(AActor* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	// 通过子类精确类查找桶（确保不同子类不会混用同一个桶）。
	TSubclassOf<AActor> ProjectileClass = Projectile->GetClass();
	FActorPool* Pool = Pools.Find(ProjectileClass);
	if (!Pool)
	{
		// 不在池里（直接 SpawnActor 出来的旧对象），直接销毁避免泄漏。
		Projectile->Destroy();
		return;
	}

	// 调用接口：子类会在这里停组件、隐藏、关碰撞。
	if (Projectile->Implements<UProjectilePoolableInterface>())
	{
		IProjectilePoolableInterface::Execute_OnReleasedToPool(Projectile);
	}

	Pool->FreeActors.AddUnique(Projectile);

	// 把它挪到地图外远点，防止 Free 状态下被玩家看到或被射线打到。
	Projectile->SetActorLocation(FVector(0.f, 0.f, -100000.f), false, nullptr, ETeleportType::TeleportPhysics);
}

void UWProjectilePoolSubsystem::DestroyPoolContents()
{
	for (TPair<TSubclassOf<AActor>, FActorPool>& Pair : Pools)
	{
		for (TWeakObjectPtr<AActor>& WeakActor : Pair.Value.AllActors)
		{
			if (AActor* Actor = WeakActor.Get())
			{
				if (Actor->Implements<UProjectilePoolableInterface>())
				{
					IProjectilePoolableInterface::Execute_OnRemovedFromPool(Actor);
				}
				Actor->Destroy();
			}
		}
	}
	Pools.Empty();
}

void UWProjectilePoolSubsystem::DumpPoolStats() const
{
	UE_LOG(LogWProjectilePool, Display, TEXT("=== ProjectilePool Stats ==="));
	for (const TPair<TSubclassOf<AActor>, FActorPool>& Pair : Pools)
	{
		const int32 Total = Pair.Value.AllActors.Num();
		const int32 Free = Pair.Value.FreeActors.Num();
		UE_LOG(LogWProjectilePool, Display, TEXT("  '%s': Active=%d Free=%d Total=%d"),
			*Pair.Key->GetName(), Total - Free, Free, Total);
	}
	UE_LOG(LogWProjectilePool, Display, TEXT("============================"));
}