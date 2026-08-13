#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Retreat.generated.h"

struct FBTRetreatTaskMemory
{
    bool bHasReachedDestination;
};

UCLASS()
class S1MPLEFPS_API UBTTask_Retreat : public UBTTaskNode
{
    GENERATED_BODY()
public:
    UBTTask_Retreat();

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RetreatDistance = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float SearchRadius = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float AcceptableRadius = 100.0f;

    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime) override;
    virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory) override;
    virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTRetreatTaskMemory); }
};
