// Fill out your copyright notice in the Description page of Project Settings.


#include "BTDecorator_HasAmmo.h"
#include "AIController.h"
#include "EnemyCharacter.h"

bool UBTDecorator_HasAmmo::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
    AAIController* AI = OwnerComp.GetAIOwner();
    if (!AI) return false;

    AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AI->GetPawn());
    if (!Enemy) return false;

    return Enemy->CurrentAmmo > 0;
}
