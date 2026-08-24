// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Investigate.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "AIController.h"
#include "NavigationSystem.h"

UBTTask_Investigate::UBTTask_Investigate()
{
	bNotifyTick = true;
	NodeName = (TEXT("Investigate"));
}

EBTNodeResult::Type UBTTask_Investigate::ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{    
	

	AEnemyAIController* AI = Cast<AEnemyAIController>(OwnerComponent.GetAIOwner());
	if (!AI) {  return EBTNodeResult::Failed; }
	UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();
	if (!BB) {  return EBTNodeResult::Failed; }
	FVector SearchLocation = FVector::ZeroVector;
	bool bHasTarget = false;
	FVector LastSeen = BB->GetValueAsVector("LastSeenLocation");
	FVector LastDamage = BB->GetValueAsVector("LastDamageLocation");
	FVector LastHeard = BB->GetValueAsVector("LastHeardLocation");
	if (!LastSeen.IsNearlyZero()) {
		SearchLocation = LastSeen;
		bHasTarget = true;
	}
	else if (!LastDamage.IsNearlyZero()) {
		SearchLocation = LastDamage;
		bHasTarget = true;
	}
	else if (!LastHeard.IsNearlyZero()) {
		SearchLocation = LastHeard;
		bHasTarget = true;
	}
	if (!bHasTarget) return EBTNodeResult::Failed;

	

	AI->MoveToLocation(SearchLocation, RadiusAccept, true);

	FBTInvestigateTaskMemory* Memory = reinterpret_cast<FBTInvestigateTaskMemory*>(NodeMemory);
	Memory->bResearchLoaction = false;
	Memory->SerachTimeRemaining = SerachDuration;

	
	return EBTNodeResult::InProgress;
}

void UBTTask_Investigate::TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime)
{
	AAIController* AI = OwnerComponent.GetAIOwner();
	UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();

	if (!AI || !BB)
	{
		FinishLatentTask(OwnerComponent, EBTNodeResult::Failed);
		return;
	}

	FBTInvestigateTaskMemory* Memory = reinterpret_cast<FBTInvestigateTaskMemory*>(NodeMemory);

	// 移动阶段：等 AI 走到目标点
	if (!Memory->bResearchLoaction)
	{
		if (!AI->IsFollowingAPath())
		{
			Memory->bResearchLoaction = true;
		}
		return;
	}

	// 搜索阶段：原地观察一段时间
	Memory->SerachTimeRemaining -= DeltaTime;
	if (Memory->SerachTimeRemaining <= 0.0f)
	{
		// 调查结束清空记忆位置，避免重复调查同一点
		BB->SetValueAsVector("LastSeenLocation", FVector::ZeroVector);
		BB->SetValueAsVector("LastDamageLocation", FVector::ZeroVector);
		BB->SetValueAsVector("LastHeardLocation", FVector::ZeroVector);

		FinishLatentTask(OwnerComponent, EBTNodeResult::Succeeded);
	}
}

EBTNodeResult::Type UBTTask_Investigate::AbortTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{   
	AAIController* AI = OwnerComponent.GetAIOwner();
	if (AI) {
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}
