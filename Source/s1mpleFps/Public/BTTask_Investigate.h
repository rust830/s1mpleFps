// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Investigate.generated.h"

/**
 * 
 */
struct FBTInvestigateTaskMemory {
	bool bResearchLoaction;
	float SerachTimeRemaining;
};
UCLASS()
class S1MPLEFPS_API UBTTask_Investigate : public UBTTaskNode
{
	GENERATED_BODY()
public:
	UBTTask_Investigate();
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float SerachDuration = 3.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float RadiusAccept = 50.0f;
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTInvestigateTaskMemory); }
	
};
