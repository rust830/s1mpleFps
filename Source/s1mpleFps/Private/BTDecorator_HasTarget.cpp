// Fill out your copyright notice in the Description page of Project Settings.


#include "BTDecorator_HasTarget.h"
#include "BehaviorTree/BlackboardComponent.h"

bool UBTDecorator_HasTarget::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemoey) const
{
	UBlackboardComponent* BB = OwnerComponent.GetBlackboardComponent();
	if (!BB)return false;

	return (BB->GetValueAsObject("TargetActor")!=nullptr);
}
