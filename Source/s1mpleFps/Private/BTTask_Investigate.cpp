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
	UE_LOG(LogTemp, Warning, TEXT("BTTask_Investigate: ExecuteTask called"));

	AEnemyAIController* AI = Cast<AEnemyAIController>(OwnerComponent.GetAIOwner());
	if (!AI) { UE_LOG(LogTemp, Warning, TEXT("BTTask_Investigate: AI cast failed")); return EBTNodeResult::Failed; }
	UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();
	if (!BB) { UE_LOG(LogTemp, Warning, TEXT("BTTask_Investigate: BB is null")); return EBTNodeResult::Failed; }
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

	UE_LOG(LogTemp, Warning, TEXT("BTTask_Investigate: Moving to %s"), *SearchLocation.ToString());

	AI->MoveToLocation(SearchLocation, RadiusAccept, true);

	FBTInvestigateTaskMemory* Memory = reinterpret_cast<FBTInvestigateTaskMemory*>(NodeMemory);
	Memory->bResearchLoaction = false;
	Memory->SerachTimeRemaining = SerachDuration;

	UE_LOG(LogTemp, Warning, TEXT("BTTask_Investigate: MoveTo started, returning InProgress"));
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

	// �ƶ��׶Σ��� AI �ߵ�Ŀ���
	if (!Memory->bResearchLoaction)
	{
		if (!AI->IsFollowingAPath())
		{
			Memory->bResearchLoaction = true;
		}
		return;
	}

	// �����׶Σ�ԭ����������ʱ
	Memory->SerachTimeRemaining -= DeltaTime;
	if (Memory->SerachTimeRemaining <= 0.0f)
	{
		// ��������������ѵ����λ�ã������ظ�����ͬһ��
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
