#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_Fire.generated.h"

struct FBTFireServiceMemory
{
	float TimeSinceLastShot;
};

UCLASS()
class S1MPLEFPS_API UBTService_Fire : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_Fire();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fire")
	float FireInterval = 0.2f;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaTime) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTFireServiceMemory); }
};
