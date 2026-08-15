#include "ChatWidget.h"
#include "s1mpleFpsPlayerController.h"

void UChatWidget::SetOwningPC(As1mpleFpsPlayerController* InPC)
{
	OwningPC = InPC;
}

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
	if (As1mpleFpsPlayerController* PC = OwningPC.Get())
	{
		PC->OnChatTextCommitted(Text, CommitMethod);
	}
}
