 // Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateHealthPercent.generated.h"

/**
 * 行为树服务：每帧读取 AI 控制角色的血量百分比，写入黑板指定 Key
 * 供行为树判断"血量低时逃跑/触发其他行为"
 */
UCLASS()
class WARRIOR_API UBTService_UpdateHealthPercent : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateHealthPercent();

	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** 黑板中用于存储血量百分比(0~1)的 Key */
	UPROPERTY(EditAnywhere, Category = "Health")
	FBlackboardKeySelector HealthPercentKey;
};
