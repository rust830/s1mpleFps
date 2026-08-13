#pragma once
#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "EnemyAIController.h"
#include "BTDecorator_CheckAIState.generated.h"

UCLASS()
class S1MPLEFPS_API UBTDecorator_CheckAIState : public UBTDecorator_BlackboardBase
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckAIState();

	UPROPERTY(EditAnywhere, Category = Blackboard)
	FBlackboardKeySelector CurrentStateKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	EAIState ExpectedState = EAIState::Combat;

	virtual bool CalculateRawConditionValue(
		UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void InitializeFromAsset(UBehaviorTree& Asset) override;
};
