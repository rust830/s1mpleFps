// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Reload.generated.h"

/**
 * 
 */
struct FBTReloadMemoryNode {
	float TimeRemaining;
	bool bHasReloaded;
};
UCLASS()
class S1MPLEFPS_API UBTTask_Reload : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_Reload();
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime)override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTReloadMemoryNode); }
	
};
