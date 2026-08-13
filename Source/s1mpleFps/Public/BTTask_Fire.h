// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Fire.generated.h"

/**
 *
 */

struct FBTFireTaskMemory {
	float TimeRemaining;
	float TimeSinceLastShot;
};
UCLASS()
class S1MPLEFPS_API UBTTask_Fire : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float BurstTime = 1.5f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float Interval = 0.15f;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTFireTaskMemory); }

	UBTTask_Fire();
};
