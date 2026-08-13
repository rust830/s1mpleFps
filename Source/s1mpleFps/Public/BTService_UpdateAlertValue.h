// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateAlertValue.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API UBTService_UpdateAlertValue : public UBTService
{
	GENERATED_BODY()
public:
	UBTService_UpdateAlertValue();

	virtual void TickNode(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemery, float DeltaTime)override;
	
};
