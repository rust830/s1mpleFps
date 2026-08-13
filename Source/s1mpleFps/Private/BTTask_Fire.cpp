// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Fire.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyAIController.h"

EBTNodeResult::Type UBTTask_Fire::ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{   
	AAIController* AI = Cast<AAIController>(OwnerComponent.GetOwner());
	if (!AI) { UE_LOG(LogTemp, Error, TEXT("BTTask_Fire: Owner not AIController")); return EBTNodeResult::Failed; }
	AEnemyAIController* Enemy = Cast<AEnemyAIController>(AI);
	if (!Enemy) { UE_LOG(LogTemp, Error, TEXT("BTTask_Fire: Cast to AEnemyAIController failed")); return EBTNodeResult::Failed; }
	UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(BB ? BB->GetValueAsObject("TargetActor") : nullptr);
	if (!Target) { UE_LOG(LogTemp, Error, TEXT("BTTask_Fire: No TargetActor in BB")); return EBTNodeResult::Failed; }
	if (!BB->GetValueAsBool("HasLineOfSight")) return EBTNodeResult::Failed;
	FBTFireTaskMemory* Memory = reinterpret_cast<FBTFireTaskMemory*>(NodeMemory);
	Memory->TimeRemaining = BurstTime;
	Memory->TimeSinceLastShot = 0.0f;
	Enemy->EnemyFire();

	
	return EBTNodeResult::InProgress;
}

void UBTTask_Fire::TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime)
{
	AAIController* AI = Cast<AAIController>(OwnerComponent.GetOwner());
	if (!AI) { FinishLatentTask(OwnerComponent, EBTNodeResult::Failed); return; }
	AEnemyAIController* Enemy = Cast<AEnemyAIController>(AI);
	if (!Enemy) { FinishLatentTask(OwnerComponent, EBTNodeResult::Failed); return; }
	UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();
	AActor* Target = Cast<AActor>(BB ? BB->GetValueAsObject("TargetActor") : nullptr);
	if (!Target) { FinishLatentTask(OwnerComponent, EBTNodeResult::Failed); return; }
	if (!BB->GetValueAsBool("HasLineOfSight")) { FinishLatentTask(OwnerComponent, EBTNodeResult::Failed); return; }
	FBTFireTaskMemory* Memory = reinterpret_cast<FBTFireTaskMemory*>(NodeMemory);
	Memory->TimeRemaining -= DeltaTime;
	Memory->TimeSinceLastShot += DeltaTime;
	if (Memory->TimeRemaining <= 0.0f) {
		FinishLatentTask(OwnerComponent, EBTNodeResult::Succeeded);
		return;

	}
	if (Memory->TimeSinceLastShot >= Interval) {
		Memory->TimeSinceLastShot = 0.0f;
		Enemy->EnemyFire();
	}
}

UBTTask_Fire::UBTTask_Fire()
{
	bNotifyTick = true;
	NodeName = (TEXT("Fire"));
   

}
