// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_FindCover.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "AIController.h"

UBTTask_FindCover::UBTTask_FindCover()
{
	NodeName = TEXT("Find Cover (EQS)");
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_FindCover::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{   
	AAIController*AI = Cast<AAIController>(OwnerComp.GetOwner());
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB || !AI || !CoverQuery)return EBTNodeResult::Failed;

	FFindCoverMemory* Memory = reinterpret_cast<FFindCoverMemory*>(NodeMemory);
	Memory->bMovingToCover = false;
	Memory->bHoldingCover = false;
	Memory->HoldingTime = 0.0f;
	// 记录 EQS 查询时的目标位置
	if (AActor* Target = Cast<AActor>(BB->GetValueAsObject("TargetActor"))) {
		Memory->EnemyLocationAtQuery = Target->GetActorLocation();
	}
	APawn* Pawn = AI->GetPawn();
	if (!Pawn)return EBTNodeResult::Failed;
	FEnvQueryRequest QueryRequest(CoverQuery, Pawn);
	int32 ReqID = QueryRequest.Execute(
		EEnvQueryRunMode::SingleResult,
		FQueryFinishedSignature::CreateLambda(
			[this, &OwnerComp, NodeMemory, Memory](TSharedPtr<FEnvQueryResult> Result)
			{
				
				if (Memory->QueryIndex == INDEX_NONE) return;
				OnQueryFinished(Result, &OwnerComp, Memory);
			}
		)
	);
	Memory->QueryIndex = ReqID;
	return EBTNodeResult::InProgress;
}

void UBTTask_FindCover::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{   
	AAIController* AI = OwnerComp.GetAIOwner();
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB || !AI) {
		FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
		return;
	}
	FFindCoverMemory* Memory = reinterpret_cast<FFindCoverMemory*>(NodeMemory);
	if (Memory->QueryIndex != INDEX_NONE)return;
	if (Memory->bMovingToCover && Memory->bHoldingCover) {
		if (AActor* Target = Cast<AActor>(BB->GetValueAsObject("TargetActor"))) {
			float Distance = FVector::Dist(Target->GetActorLocation(), Memory->EnemyLocationAtQuery);
			if (Distance > RepositionThrehold) {
				AI->StopMovement();
				FFindCoverMemory* Mem = reinterpret_cast<FFindCoverMemory*>(NodeMemory);
				Mem->bMovingToCover = false;
				ExecuteTask(OwnerComp, NodeMemory);
				return;
			}
		}
		
		}
	if (AI->GetMoveStatus() == EPathFollowingStatus::Idle) {
		Memory->bHoldingCover = true;
		Memory->HoldingTime = 0.0f;
		return;
	}
	if (Memory->bHoldingCover) {
		Memory->HoldingTime += DeltaTime;
		if (Memory->HoldingTime >= Memory->StayTime) {
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
		return;
	}
}

EBTNodeResult::Type UBTTask_FindCover::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{   
	if (AAIController* AI = Cast<AAIController>(OwnerComp.GetOwner())) {
		AI->StopMovement();
	}
	return EBTNodeResult::Aborted;
}

void UBTTask_FindCover::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result, UBehaviorTreeComponent* OwnerComp, FFindCoverMemory* Memory)
{
	if (!Result || !Result->IsSuccessful() || Result->Items.Num() == 0) {
		FinishLatentTask(*OwnerComp, EBTNodeResult::Succeeded);
		return;
	}
	FVector CoverLocation = Result->GetItemAsLocation(0);
	if (AAIController* AI = OwnerComp->GetAIOwner())
	{
		if (APawn* Pawn = AI->GetPawn())
		{
			CoverLocation.Z = Pawn->GetActorLocation().Z;
		}
	}
	AAIController* AI = OwnerComp->GetAIOwner();
	if (!AI) {
		FinishLatentTask(*OwnerComp, EBTNodeResult::Failed);
		return;
	}
	if (UBlackboardComponent* BB = OwnerComp->GetBlackboardComponent()) {
		BB->SetValueAsVector(CoverLocationKey, CoverLocation);

	}
	StartMovingToCover(AI, CoverLocation, *OwnerComp, Memory);
}

void UBTTask_FindCover::StartMovingToCover(AAIController* AI, const FVector& CoverLocation, UBehaviorTreeComponent& OwnerComp, FFindCoverMemory* Memory)
{

	if (!AI)return;
	AI->MoveToLocation(CoverLocation, AcceptedDistance, true);
	Memory->bHoldingCover = false;
	Memory->bMovingToCover = true;
	Memory->StayTime = FMath::FRandRange(minTime, maxTime);
	return;
}
