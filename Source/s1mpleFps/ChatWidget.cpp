#include "ChatWidget.h"
#include "s1mpleFpsPlayerController.h"

void UChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddDynamic(this, &UChatWidget::OnChatTextCommittedHandler);
		ChatInput->SetKeyboardFocus();
	}
}

void UChatWidget::OnChatTextCommittedHandler(const FText& Text, ETextCommit::Type CommitMethod)
{
	As1mpleFpsPlayerController* PC = GetOwningPlayer<As1mpleFpsPlayerController>();
	if (PC)
	{
		PC->OnChatTextCommitted(Text, CommitMethod);
	}
}
