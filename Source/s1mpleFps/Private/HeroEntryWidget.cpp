// Fill out your copyright notice in the Description page of Project Settings.


#include "HeroEntryWidget.h"
#include "s1mpleFpsPlayerController.h"

void UHeroEntryWidget::SetupHero(UHeroData* InHero, int32 InIndex)
{
	// 这里只存数据，不碰 BindWidget（此刻 BindWidget 成员还没绑定）
	Hero = InHero;
	HeroIndex = InIndex;
}

void UHeroEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidget 成员此时已就位，套用 SetupHero 存下的数据
	if (PortraitImage && Hero && Hero->Portrait)
	{
		PortraitImage->SetBrushFromTexture(Hero->Portrait);
	}
	if (NameText && Hero)
	{
		NameText->SetText(Hero->DisplayName);
	}

	if (CardButton)
	{
		CardButton->OnClicked.AddDynamic(this, &UHeroEntryWidget::OnCardClicked);
	}
	if (HighlightImage)
	{
		HighlightImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHeroEntryWidget::SetSelected(bool bSelected)
{
	if (!HighlightImage) return;
	// 状态没变就跳过：定时器每 0.25s 调一次，重复设置同样的可见性/颜色会导致高亮闪烁
	if (bSelected == bIsSelected) return;
	bIsSelected = bSelected;

	if (bSelected)
	{
		HighlightImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		HighlightImage->SetColorAndOpacity(HighlightColor);
	}
	else
	{
		HighlightImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UHeroEntryWidget::OnCardClicked()
{
	if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetOwningPlayer()))
	{
		PC->ServerSelectHero(HeroIndex);
	}
}
