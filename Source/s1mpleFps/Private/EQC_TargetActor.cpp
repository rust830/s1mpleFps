// Fill out your copyright notice in the Description page of Project Settings.


#include "EQC_TargetActor.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "AIController.h"

void UEQC_TargetActor::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* Query = Cast<AActor>(QueryInstance.Owner.Get());
	if (!Query)return;
	AAIController* AI = Cast<AAIController>(Query);
	if (!AI)return;
	UBlackboardComponent* BB = AI->GetBlackboardComponent();
	if (!BB)return;
	AActor* Target = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
	if (Target) {
		UEnvQueryItemType_Actor::SetContextHelper(ContextData, Target);
	}
}
