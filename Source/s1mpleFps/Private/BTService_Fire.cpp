#include "BTService_Fire.h"
#include "EnemyAIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_Fire::UBTService_Fire()
{
	NodeName = TEXT("Fire Service");
	Interval = 0.05f;
	bCallTickOnSearchStart = true;
}

void UBTService_Fire::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaTime)
{
	FBTFireServiceMemory* Memory = reinterpret_cast<FBTFireServiceMemory*>(NodeMemory);

	AEnemyAIController* AI = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	if (!AI) return;

	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return;

	AActor* Target = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
	if (!Target) return;

	if (!BB->GetValueAsBool("HasLineOfSight")) return;

	Memory->TimeSinceLastShot += DeltaTime;
	if (Memory->TimeSinceLastShot >= FireInterval)
	{
		Memory->TimeSinceLastShot = 0.0f;
		AI->EnemyFire();
	}
}
