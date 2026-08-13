#include "BTTask_Retreat.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "EnemyAIController.h"
#include "NavigationSystem.h"
#include "AIController.h"

UBTTask_Retreat::UBTTask_Retreat()
{
    bNotifyTick = true;
    NodeName = TEXT("Retreat");
}

EBTNodeResult::Type UBTTask_Retreat::ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{
    AAIController* AI = OwnerComponent.GetAIOwner();
    if (!AI) return EBTNodeResult::Failed;

    UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();
    if (!BB) return EBTNodeResult::Failed;

    APawn* MyPawn = AI->GetPawn();
    if (!MyPawn) return EBTNodeResult::Failed;

    AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject("TargetActor"));

    FVector RetreatLocation;
    bool bFoundPoint = false;

    if (TargetActor)
    {
        FVector AwayFromTarget = (MyPawn->GetActorLocation() - TargetActor->GetActorLocation()).GetSafeNormal2D();
        FVector DesiredPoint = MyPawn->GetActorLocation() + AwayFromTarget * RetreatDistance;

        UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());
        if (NavSys)
        {
            FNavLocation NavLocation;
            bFoundPoint = NavSys->GetRandomPointInNavigableRadius(DesiredPoint, SearchRadius, NavLocation);
            if (bFoundPoint)
            {
                RetreatLocation = NavLocation.Location;
            }
        }
    }

    if (!bFoundPoint)
    {
        FVector Backward = -MyPawn->GetActorForwardVector().GetSafeNormal2D();
        RetreatLocation = MyPawn->GetActorLocation() + Backward * RetreatDistance;
    }

    UE_LOG(LogTemp, Warning, TEXT("BTTask_Retreat: Moving to %s (NavMesh: %s)"),
        *RetreatLocation.ToString(), bFoundPoint ? TEXT("yes") : TEXT("no"));

    AI->MoveToLocation(RetreatLocation, AcceptableRadius, true);

    FBTRetreatTaskMemory* Memory = reinterpret_cast<FBTRetreatTaskMemory*>(NodeMemory);
    Memory->bHasReachedDestination = false;

    return EBTNodeResult::InProgress;
}

void UBTTask_Retreat::TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime)
{
    AAIController* AI = OwnerComponent.GetAIOwner();
    if (!AI)
    {
        FinishLatentTask(OwnerComponent, EBTNodeResult::Failed);
        return;
    }

    FBTRetreatTaskMemory* Memory = reinterpret_cast<FBTRetreatTaskMemory*>(NodeMemory);

    if (!AI->IsFollowingAPath() && !Memory->bHasReachedDestination)
    {
        Memory->bHasReachedDestination = true;
        FinishLatentTask(OwnerComponent, EBTNodeResult::Succeeded);
    }
}

EBTNodeResult::Type UBTTask_Retreat::AbortTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{
    AAIController* AI = OwnerComponent.GetAIOwner();
    if (AI)
    {
        AI->StopMovement();
    }
    return EBTNodeResult::Aborted;
}
