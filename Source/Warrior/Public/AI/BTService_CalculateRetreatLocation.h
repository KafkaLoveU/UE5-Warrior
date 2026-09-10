// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_CalculateRetreatLocation.generated.h"

/**
 * 行为树服务：计算"远离目标"的逃跑点位置，写入黑板 Vector Key
 * 逻辑：逃跑点 = 自身位置 + (自身位置 - 目标位置).法线 * 逃跑距离
 */
UCLASS()
class WARRIOR_API UBTService_CalculateRetreatLocation : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_CalculateRetreatLocation();

	//~ Begin UBTNode Interface
	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
	virtual FString GetStaticDescription() const override;
	//~ End UBTNode Interface

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

protected:
	/** 黑板中记录目标的 Key（逃跑时远离它） */
	UPROPERTY(EditAnywhere, Category = "Retreat")
	FBlackboardKeySelector TargetActorKey;

	/** 黑板中写入逃跑点位置的 Key（Vector 类型） */
	UPROPERTY(EditAnywhere, Category = "Retreat")
	FBlackboardKeySelector RetreatLocationKey;

	/** 逃跑距离（厘米） */
	UPROPERTY(EditAnywhere, Category = "Retreat")
	float RetreatDistance = 800.f;
};
