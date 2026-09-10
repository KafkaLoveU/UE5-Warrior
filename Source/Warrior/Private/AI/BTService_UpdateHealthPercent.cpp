// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/BTService_UpdateHealthPercent.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AbilitySystemComponent.h"

#include "AbilitySystem/WAttributeSet.h"
#include "Characters/WBaseCharacter.h"

UBTService_UpdateHealthPercent::UBTService_UpdateHealthPercent()
{
	NodeName = TEXT("Native Update Health Percent");

	// ★ 必须调用：开启服务的 Tick 通知，否则 TickNode 不会被调用
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	// 每 0.2s 读一次即可：血量变化不需要逐帧同步，
	// 且逐帧写黑板会让挂在服务上的 Aborts 装饰器每帧重新求值、引起行为树抖动
	Interval		= 0.2f;
	RandomDeviation = 0.f;

	// 限定该 Key 只能选择 Float 类型的黑板条目
	HealthPercentKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, HealthPercentKey));
}

// C++ 中定义的 FBlackboardKeySelector 必须在此完成 Key 的解析绑定
void UBTService_UpdateHealthPercent::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (UBlackboardData* BBAsset = GetBlackboardAsset())
	{
		HealthPercentKey.ResolveSelectedKey(*BBAsset);
	}
}

FString UBTService_UpdateHealthPercent::GetStaticDescription() const
{
	return FString::Printf(TEXT("Update health percent into Blackboard Key: %s"), *HealthPercentKey.SelectedKeyName.ToString());
}

void UBTService_UpdateHealthPercent::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// 1. 通过 AI Controller 拿到它控制的 Pawn
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

	// 2. 拿角色的 AttributeSet
	AWBaseCharacter* BaseCharacter = Cast<AWBaseCharacter>(ControlledPawn);
	if (!BaseCharacter)
	{
		return;
	}

	// ★ 必须从 ASC 取，不能用基类的 GetWAttributeSet() 成员指针。
	// GE（含初始化用的 GE_XXX_StartUp / GE_Enemy_Static）只会作用在
	// ASC->GetSpawnedAttributes() 中注册的那个 AttributeSet 上；而角色身上可能存在
	// 多个 AttributeSet 组件实例，基类成员指针未必是 ASC 实际注册的那一个。
	UAbilitySystemComponent* ASC = BaseCharacter->GetAbilitySystemComponent();
	const UWAttributeSet* AttributeSet = ASC ? ASC->GetSet<UWAttributeSet>() : nullptr;

	// 兜底：ASC 未注册时退回基类成员，避免服务静默失效
	if (!AttributeSet)
	{
		AttributeSet = BaseCharacter->GetWAttributeSet();
	}

	if (!AttributeSet)
	{
		return;
	}

	// 3. 计算血量百分比 (0~1)，防止除零
	const float MaxHealth = AttributeSet->GetMaxHealth();
	if (MaxHealth <= 0.f)
	{
		return;
	}

	const float CurrentHealth = AttributeSet->GetCurrentHealth();
	const float HealthPercent = FMath::Clamp(CurrentHealth / MaxHealth, 0.f, 1.f);

	// 4. 写入黑板
	if (UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent())
	{
		BlackboardComp->SetValueAsFloat(HealthPercentKey.SelectedKeyName, HealthPercent);
	}
}