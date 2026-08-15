#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "ChatWidget.generated.h"

class As1mpleFpsPlayerController;

UCLASS()
class S1MPLEFPS_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetOwningPC(As1mpleFpsPlayerController* InPC);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInput;

	UFUNCTION()
	void OnChatTextCommittedHandler(const FText& Text, ETextCommit::Type CommitMethod);

private:
	TWeakObjectPtr<As1mpleFpsPlayerController> OwningPC;
};
