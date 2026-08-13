#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "ChatWidget.generated.h"

UCLASS()
class S1MPLEFPS_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInput;

	UFUNCTION()
	void OnChatTextCommittedHandler(const FText& Text, ETextCommit::Type CommitMethod);
};
