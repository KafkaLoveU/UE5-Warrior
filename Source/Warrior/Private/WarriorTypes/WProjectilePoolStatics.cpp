// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#include "WarriorTypes/WProjectilePoolStatics.h"

#include "Subsystems/WProjectilePoolSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AActor* UWProjectilePoolStatics::SpawnProjectileFromPool(UObject* WorldContextObject,
                                                        TSubclassOf<AActor> ProjectileClass,
                                                        FVector Location,
                                                        FRotator Rotation,
                                                        AActor* Instigator,
                                                        const FGameplayEffectSpecHandle& DamageSpecHandle)
{
	// 入口日志：无论成败先留痕，用于排查"节点是否被调用 / 传参是什么"。
	UE_LOG(LogWProjectilePool, Display,
		TEXT("[ProjectilePool] SpawnProjectileFromPool called. Class='%s' Loc=%s Rot=%s"),
		ProjectileClass ? *ProjectileClass->GetName() : TEXT("NULL"),
		*Location.ToString(), *Rotation.ToString());

	if (!WorldContextObject || !ProjectileClass)
	{
		UE_LOG(LogWProjectilePool, Warning, TEXT("[ProjectilePool] SpawnProjectileFromPool aborted: WorldContext or Class is null."));
		return nullptr;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		UE_LOG(LogWProjectilePool, Warning, TEXT("[ProjectilePool] SpawnProjectileFromPool aborted: could not resolve World from context."));
		return nullptr;
	}

	// 在 C++ 侧把 Location/Rotation 拼成 FTransform，蓝图侧保持和 SpawnActor 相同的引脚形态。
	const FTransform SpawnTransform(Rotation, Location);

	UWProjectilePoolSubsystem* Pool = World->GetSubsystem<UWProjectilePoolSubsystem>();
	if (!Pool)
	{
		UE_LOG(LogWProjectilePool, Warning, TEXT("[ProjectilePool] SpawnProjectileFromPool aborted: Pool subsystem not found."));
		return nullptr;
	}

	return Pool->AcquireProjectile(ProjectileClass, SpawnTransform, Instigator, DamageSpecHandle);
}

void UWProjectilePoolStatics::ReturnProjectileToPool(UObject* WorldContextObject, AActor* Projectile)
{
	if (!WorldContextObject || !IsValid(Projectile))
	{
		return;
	}

	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return;
	}

	if (UWProjectilePoolSubsystem* Pool = World->GetSubsystem<UWProjectilePoolSubsystem>())
	{
		Pool->ReleaseProjectile(Projectile);
	}
}