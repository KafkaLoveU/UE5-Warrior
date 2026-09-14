// Copyright (c) 2026. Learning Demo: Object Pool for Projectiles.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayEffectTypes.h"
#include "ProjectilePoolableInterface.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UProjectilePoolableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 池化协议：任何想被池化的投掷物都必须实现这三个方法。
 *
 * 关键点（本次修复）：每个方法必须标 UFUNCTION(BlueprintNativeEvent)。
 * 原因：UHT 只为带 UFUNCTION 的接口方法生成两样东西：
 *   1. Execute_xxx 静态桥接函数（调用方用，兼容 C++/蓝图双实现）
 *   2. xxx_Implementation 虚函数（C++ 实现方重载的目标）
 * 纯 C++ 虚函数（= 0）两者都不会生成，导致编译报错。
 *
 * 调用时序：
 *   Acquire:  Execute_OnAcquiredFromPool()  → 进入 Active 状态
 *   Release:  Execute_OnReleasedToPool()    → 进入 Inactive 状态
 *   Deinit:   Execute_OnRemovedFromPool()   → 池被销毁时调用（关卡切换）
 */
class WARRIOR_API IProjectilePoolableInterface
{
	GENERATED_BODY()

public:
	/** 池里取出时被调用：设置 Transform、伤害句柄、组件激活、寿命定时器等。 */
	UFUNCTION(BlueprintNativeEvent)
	void OnAcquiredFromPool(const FTransform& InTransform, const FGameplayEffectSpecHandle& InDamageSpec);

	/** 回池时被调用：停 Niagara、停 ProjectileMovement、隐藏、关碰撞、清状态。 */
	UFUNCTION(BlueprintNativeEvent)
	void OnReleasedToPool();

	/** 池销毁时调用：真正走 Destroy() 释放内存。 */
	UFUNCTION(BlueprintNativeEvent)
	void OnRemovedFromPool();

	/** 用于按"精确子类"分桶（避免子类共享池导致类型混淆）。返回 GetClass() 即可。
	 *  注意：BlueprintNativeEvent 不支持 const 限定，这里去掉 const。 */
	UFUNCTION(BlueprintNativeEvent)
	UClass* GetPoolableProjectileClass();
};