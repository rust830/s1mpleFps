// Fill out your copyright notice in the Description page of Project Settings.


#include "FlashWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"

void UFlashWidget::StartFlashing(float Intensity, float Duration, FLinearColor Color)
{
	if (!Image) return;
	Intensity = FMath::Clamp(Intensity, 0.0f, 1.0f);
	Duration = FMath::Max(Duration, 0.01f);
	CurrentAlpha = Intensity;
	AlphaPerSecond = Intensity / Duration;
	bIsFlashing = true;
	FlashColor = Color;
	Image->SetColorAndOpacity(FLinearColor(Color.R, Color.G, Color.B, Intensity));
}

TSharedRef<SWidget> UFlashWidget::RebuildWidget()
{
	// 关键：必须在 RebuildWidget（而不是 NativeConstruct）里建树。
	// NativeConstruct 执行时 Slate 已经抓取过 WidgetTree->RootWidget，
	// 纯 C++ 在 NativeConstruct 里动态建树会晚一步，导致界面始终为空、不渲染。
	BuildFlashUI();
	return Super::RebuildWidget();
}

void UFlashWidget::BuildFlashUI()
{
	if (!WidgetTree || Image) return;  // 只建一次，避免重复构造

	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
	WidgetTree->RootWidget = Canvas;

	Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	// 默认画刷是 NoDrawType，什么都不画 —— 必须设一个能绘制的画刷，否则白闪永远不可见
	FSlateBrush FlashBrush;
	FlashBrush.DrawAs = ESlateBrushDrawType::Box;
	FlashBrush.TintColor = FLinearColor::White;
	Image->SetBrush(FlashBrush);
	Image->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));

	UCanvasPanelSlot* MySlot = Canvas->AddChildToCanvas(Image);
	MySlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	MySlot->SetOffsets(FMargin(0.0f));
	MySlot->SetZOrder(999);
}

void UFlashWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (!Image||!bIsFlashing)return;
	CurrentAlpha -= InDeltaTime * AlphaPerSecond;
	if (CurrentAlpha <= 0) {
		CurrentAlpha = 0.0f;
		bIsFlashing = false;
		Image->SetColorAndOpacity(FLinearColor(FlashColor.R, FlashColor.G, FlashColor.B, 0.0f));
		RemoveFromParent();
		return;
	}
	Image->SetColorAndOpacity(FLinearColor(FlashColor.R, FlashColor.G, FlashColor.B, CurrentAlpha));
}
