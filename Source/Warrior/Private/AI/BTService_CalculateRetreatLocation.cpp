// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_CalculateRetreatLocation.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_CalculateRetreatLocation::UBTService_CalculateRetreatLocation()
{
	NodeName = TEXT("Native Calculate Retreat Location");

	// ★ 必须调用：开启服务的 Tick 通知，否则 TickNode 不会被调用
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	Interval		= 0.2f;
	RandomDeviation = 0.05f;

	// 目标 Key 过滤：只能选 Object（Actor）类型
	TargetActorKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, TargetActorKey), AActor::StaticClass());
	// 逃跑点 Key 过滤：只能选 Vector 类型
	RetreatLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, RetreatLocationKey));
}

void UBTService_CalculateRetreatLocation::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		TargetActorKey.ResolveSelectedKey(*BBAsset);
		RetreatLocationKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTService_CalculateRetreatLocation::GetStaticDescription() const
{
	return FString::Printf(TEXT("Retreat from %s to Key %s (Dist %.0f)"),
		*TargetActorKey.SelectedKeyName.ToString(),
		*RetreatLocationKey.SelectedKeyName.ToString(),
		RetreatDistance);
}

void UBTService_CalculateRetreatLocation::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return;
	}

	// 拿目标（逃跑时远离的对象）
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetActorKey.SelectedKeyName));
	if (!TargetActor)
	{
		return;
	}

	// 计算"远离目标"的方向：自身位置 - 目标位置，归一化
	const FVector SelfLocation	 = ControlledPawn->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector RetreatDirection	 = SelfLocation - TargetLocation;

	// 同一位置时给个默认方向，避免零向量
	if (RetreatDirection.IsNearlyZero())
	{
		RetreatDirection = ControlledPawn->GetActorForwardVector() * -1.f;
	}
	RetreatDirection.Normalize();

	// 逃跑点 = 自身位置 + 反方向 * 距离
	const FVector RetreatLocation = SelfLocation + RetreatDirection * RetreatDistance;

	// 写入黑板
	BlackboardComp->SetValueAsVector(RetreatLocationKey.SelectedKeyName, RetreatLocation);
}
