#include "BTDecorator_CheckAIState.h"

UBTDecorator_CheckAIState::UBTDecorator_CheckAIState()
{
	FlowAbortMode = EBTFlowAbortMode::Both;
}

bool UBTDecorator_CheckAIState::CalculateRawConditionValue(
	UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BB = OwnerComp.GetBlackboardComponent();
	if (!BB) return false;

	const FName KeyName = CurrentStateKey.SelectedKeyName;
	if (KeyName.IsNone()) return false;

	const EAIState Current = static_cast<EAIState>(BB->GetValueAsEnum(KeyName));
	return Current == ExpectedState;
}

void UBTDecorator_CheckAIState::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);
	UBlackboardData* BBAsset = GetBlackboardAsset();
	if (BBAsset)
	{
		CurrentStateKey.ResolveSelectedKey(*BBAsset);
	}
}
