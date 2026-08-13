// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "BTTask_FindCover.generated.h"

/**
 * 
 */
struct FFindCoverMemory {
	bool bMovingToCover = false;
	bool bHoldingCover = false;
	float HoldingTime = 0.0f;
	float StayTime = 0.0f;
	FVector EnemyLocationAtQuery = FVector::ZeroVector;
	int32 QueryIndex = INDEX_NONE;
};
UCLASS()
class S1MPLEFPS_API UBTTask_FindCover : public UBTTaskNode
{
	GENERATED_BODY()
public:

	UBTTask_FindCover();
	UPROPERTY(EditAnywhere,BLueprintReadWrite)
	UEnvQuery* CoverQuery = nullptr;
	UPROPERTY(EditAnywhere, BLueprintReadWrite)
	float AcceptedDistance = 120.0f;
	UPROPERTY(EditAnywhere, BLueprintReadWrite)
	float minTime = 2.5f;
	UPROPERTY(EditAnywhere, BLueprintReadWrite)
	float maxTime = 5.5f;
	UPROPERTY(EditAnywhere, BLueprintReadWrite)
	float RepositionThrehold = 800.0f;
	UPROPERTY(EditAnywhere, BLueprintReadWrite)
	FName CoverLocationKey = "CoverLocation";
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		float DeltaTime) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp,
		uint8* NodeMemory) override;

	virtual uint16 GetInstanceMemorySize() const override
	{
		return sizeof(FFindCoverMemory);
	}

protected:
	void OnQueryFinished(TSharedPtr<FEnvQueryResult> Result,
		UBehaviorTreeComponent* OwnerComp, FFindCoverMemory* Memory);
	void StartMovingToCover(AAIController* AI, const FVector& CoverLocation,
		UBehaviorTreeComponent& OwnerComp, FFindCoverMemory* Memory);
};
