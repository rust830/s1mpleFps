// Fill out your copyright notice in the Description page of Project Settings.


#include "EQC_TargetLocation.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "AIController.h"

void UEQC_TargetLocation::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* Query = Cast<AActor>(QueryInstance.Owner.Get());
	if (!Query)return;
	AAIController* AI = Cast<AAIController>(Query);
	if (!AI)return;
	UBlackboardComponent* BB = AI->GetBlackboardComponent();
	if (!BB)return;
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
	if (TargetActor)
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData,
			TargetActor->GetActorLocation());
	}
}
