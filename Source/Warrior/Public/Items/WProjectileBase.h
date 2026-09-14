// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "Interfaces/ProjectilePoolableInterface.h"
#include "WProjectileBase.generated.h"

class UBoxComponent;
class UNiagaraComponent;
class UProjectileMovementComponent;
struct FGameplayEventData;

UENUM(BlueprintType)
enum class EProjectileDamagePolicy : uint8
{
	OnHit,
	OnBeginOverlap,
};

/**
 * 投掷物基类（已支持对象池）
 *
 * 关键改造点：
 *   1. 实现 IProjectilePoolableInterface 三方法
 *   2. 不再使用 AActor::InitialLifeSpan（那个会调用 Destroy()，绕过我们的回池）
 *   3. 用 FTimerHandle 模拟寿命，到期回池
 *   4. OnProjectileHit 中的 Destroy() 全部改为 ReturnToPool()
 */
UCLASS(Abstract)
class WARRIOR_API AWProjectileBase : public AActor, public IProjectilePoolableInterface
{
	GENERATED_BODY()

public:
	AWProjectileBase();

	//~ Begin IProjectilePoolableInterface
	virtual void OnAcquiredFromPool_Implementation(const FTransform& InTransform, const FGameplayEffectSpecHandle& InDamageSpec) override;
	virtual void OnReleasedToPool_Implementation() override;
	virtual void OnRemovedFromPool_Implementation() override;
	virtual UClass* GetPoolableProjectileClass_Implementation() override { return GetClass(); }
	//~ End IProjectilePoolableInterface

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UBoxComponent> ProjectileCollisionBox;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComp;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	EProjectileDamagePolicy ProjectileDamagePolicy = EProjectileDamagePolicy::OnHit;

	UPROPERTY(BlueprintReadOnly, Category = "Projectile", meta = (ExposeOnSpawn = "true"))
	FGameplayEffectSpecHandle ProjectileDamageEffectSpecHandle;

	UFUNCTION(BlueprintCallable)
	void SetProjectileDamageEffectSpecHandle(FGameplayEffectSpecHandle InSpecHandle) { ProjectileDamageEffectSpecHandle = InSpecHandle; }

	/** 池化寿命（默认 4 秒）；到期自动回池而非销毁。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float PoolLifeSpan = 4.f;

	UFUNCTION()
	virtual void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION()
	virtual void OnProjectileBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Spawn Projectile Hit FX"))
	void BP_OnSpawnProjectileHitFX(const FVector& HitLocation);

	/** 寿命定时器到期回调：仅回池。 */
	UFUNCTION()
	void HandleLifeSpanExpired();

	/** 把"销毁"统一收敛到这一个方法（既处理池，也处理非池的兜底）。 */
	void ReturnToPool();

private:
	void HandleApplyProjectileDamage(APawn* InHitPawn, const FGameplayEventData& InPayload);

	TArray<AActor*> OverlappedActors;

	/** 当前寿命定时器 Handle，方便重入时清掉旧定时器。 */
	FTimerHandle LifeSpanTimerHandle;

	/** 标志位：标记该 Actor 当前是否在池中"空闲"，避免 Free 时被重叠事件触发。 */
	bool bIsPooledInactive = false;
};