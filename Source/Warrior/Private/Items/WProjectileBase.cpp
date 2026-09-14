// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#include "Items/WProjectileBase.h"

#include "Components/BoxComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "WFunctionLibrary.h"
#include "WGameplayTags.h"
#include "WarriorTypes/WProjectilePoolStatics.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "TimerManager.h"
#include "Engine/World.h"

#include "WDebugHelper.h"

AWProjectileBase::AWProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileCollisionBox = CreateDefaultSubobject<UBoxComponent>("ProjectileCollisionBox");
	SetRootComponent(ProjectileCollisionBox);
	ProjectileCollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	ProjectileCollisionBox->OnComponentHit.AddUniqueDynamic(this, &ThisClass::OnProjectileHit);
	ProjectileCollisionBox->OnComponentBeginOverlap.AddUniqueDynamic(this, &ThisClass::OnProjectileBeginOverlap);

	NiagaraComp = CreateDefaultSubobject<UNiagaraComponent>("ProjectileNiagaraComponent");
	NiagaraComp->SetupAttachment(GetRootComponent());

	ProjectileMovementComp = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
	ProjectileMovementComp->InitialSpeed = 700.f;
	ProjectileMovementComp->MaxSpeed = 900.f;
	ProjectileMovementComp->Velocity = FVector(1.f, 0.f, 0.f);
	ProjectileMovementComp->ProjectileGravityScale = 0.f;

	// 关键：禁用 InitialLifeSpan！否则引擎在 4 秒后会走 Destroy()，
	// 而不是我们的回池逻辑，池化就失效了。
	InitialLifeSpan = 0.f;
}

void AWProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileDamagePolicy == EProjectileDamagePolicy::OnBeginOverlap)
	{
		ProjectileCollisionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	}
}

// ============================================================
// 池化接口实现
// ============================================================

void AWProjectileBase::OnAcquiredFromPool_Implementation(const FTransform& InTransform, const FGameplayEffectSpecHandle& InDamageSpec)
{
	// 1) 重置伤害 spec
	ProjectileDamageEffectSpecHandle = InDamageSpec;

	// 2) 重置 Transform（TAA 在地图边缘瞬移，所以用 TeleportPhysics）
	SetActorTransform(InTransform, false, nullptr, ETeleportType::TeleportPhysics);

	// 3) 激活碰撞 / 可见性
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	bIsPooledInactive = false;

	// 4) 重置 OverlappedActors（关键：避免复用时跳过同一目标）
	OverlappedActors.Reset();

	// 5) 启动 Niagara
	if (NiagaraComp)
	{
		NiagaraComp->ActivateSystem(true);
	}

	// 6) 重置并启动 ProjectileMovement
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->StopMovementImmediately();
		ProjectileMovementComp->ProjectileGravityScale = 0.f;
		// 复用路径：回池时速度已被清零，这里沿当前朝向重新赋予初速，
		// 否则第二发起投掷物会原地卡住不飞。
		ProjectileMovementComp->Velocity = GetActorForwardVector() * ProjectileMovementComp->InitialSpeed;
	}

	// 7) 启动寿命定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifeSpanTimerHandle);
		World->GetTimerManager().SetTimer(LifeSpanTimerHandle, this, &ThisClass::HandleLifeSpanExpired, PoolLifeSpan, false);
	}
}

void AWProjectileBase::OnReleasedToPool_Implementation()
{
	bIsPooledInactive = true;

	// 停 Niagara
	if (NiagaraComp)
	{
		NiagaraComp->Deactivate();
	}

	// 停 ProjectileMovement（防止回池后还在"飞"）
	if (ProjectileMovementComp)
	{
		ProjectileMovementComp->StopMovementImmediately();
	}

	// 关闭碰撞 + 隐藏
	SetActorEnableCollision(false);
	SetActorHiddenInGame(true);
	SetActorTickEnabled(false);

	// 清掉寿命定时器（避免下次 Acquire 时上一发还没结束）
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LifeSpanTimerHandle);
	}

	// 清空 OverlappedActors，避免下次复用时误判
	OverlappedActors.Reset();

	// 伤害 spec 清掉，避免持有过期引用
	ProjectileDamageEffectSpecHandle = FGameplayEffectSpecHandle();
}

void AWProjectileBase::OnRemovedFromPool_Implementation()
{
	// 池销毁时（如关卡切换），真正释放内存。
	Destroy();
}

void AWProjectileBase::HandleLifeSpanExpired()
{
	ReturnToPool();
}

void AWProjectileBase::ReturnToPool()
{
	if (bIsPooledInactive)
	{
		// 已经在池中（双重保险，避免回池 → 重叠事件 → 又调一次回池）
		return;
	}

	UWProjectilePoolStatics::ReturnProjectileToPool(this, this);
}

void AWProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bIsPooledInactive)
	{
		return; // 在池中不应该响应任何事件
	}

	BP_OnSpawnProjectileHitFX(Hit.ImpactPoint);

	APawn* HitPawn = Cast<APawn>(OtherActor);

	if (!HitPawn || !UWFunctionLibrary::IsTargetPawnHostile(GetInstigator(), HitPawn))
	{
		ReturnToPool();
		return;
	}

	bool bIsValidBlock = false;

	const bool bIsPlayerBlocking = UWFunctionLibrary::DoesActorHaveTag(HitPawn, WTags::Player_Status_Blocking);

	if (bIsPlayerBlocking)
	{
		bIsValidBlock = UWFunctionLibrary::IsValidBlock(this, HitPawn);
	}

	FGameplayEventData Data;
	Data.Instigator = GetInstigator();
	Data.Target = HitPawn;

	if (bIsValidBlock)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitPawn, WTags::Player_Event_SuccessfulBlock, Data);
	}
	else
	{
		HandleApplyProjectileDamage(HitPawn, Data);
	}

	ReturnToPool();
}

void AWProjectileBase::OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bIsPooledInactive)
	{
		return;
	}

	if (OverlappedActors.Contains(OtherActor))
	{
		return;
	}

	OverlappedActors.AddUnique(OtherActor);

	if (APawn* HitPawn = Cast<APawn>(OtherActor))
	{
		FGameplayEventData Data;
		Data.Instigator = GetInstigator();
		Data.Target = HitPawn;

		if (UWFunctionLibrary::IsTargetPawnHostile(GetInstigator(), HitPawn))
		{
			HandleApplyProjectileDamage(HitPawn, Data);
		}
	}
}

void AWProjectileBase::HandleApplyProjectileDamage(APawn* InHitPawn, const FGameplayEventData& InPayload)
{
	checkf(ProjectileDamageEffectSpecHandle.IsValid(), TEXT("Forgot to assign a valid spec handle to the projectile: %s"), *GetActorNameOrLabel());

	const bool bWasApplied = UWFunctionLibrary::ApplyGameplayEffectSpecHandleToTargetActor(GetInstigator(), InHitPawn, ProjectileDamageEffectSpecHandle);

	if (bWasApplied)
	{
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(InHitPawn, WTags::Shared_Event_HitReact, InPayload);
	}
}