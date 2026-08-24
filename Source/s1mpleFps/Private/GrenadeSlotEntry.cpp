// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeSlotEntry.h"
#include "GrenadeData.h"
#include "Components/CanvasPanelSlot.h"

void UGrenadeSlotEntry::SetupSlot(UGrenadeData* InData, int32 InCount, bool bInHighlighted)
{
	// 这里只存数据，不碰 BindWidget（此刻 BindWidget 成员还没绑定）
	GrenadeData = InData;
	Count = InCount;
	bHighlighted = bInHighlighted;
}

void UGrenadeSlotEntry::NativeConstruct()
{
	Super::NativeConstruct();

	// BindWidget 成员此时已就位，套用 SetupSlot 存下的数据
	if (GrenadeIconImage && GrenadeData && GrenadeData->GrenadeIcon)
	{
		GrenadeIconImage->SetBrushFromTexture(GrenadeData->GrenadeIcon);
		GrenadeIconImage->SetDesiredSizeOverride(IconSize);

		// 直接改 CanvasPanelSlot 的尺寸，确保图标不因贴图原始分辨率/锚点被撑大。
		// （SetDesiredSizeOverride 只改期望尺寸，实际尺寸由槽位决定，这里强制关掉 SizeToContent 并定死大小）
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(GrenadeIconImage->Slot))
		{
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetSize(IconSize);
		}
	}
	if (GrenadeCountText)
	{
		GrenadeCountText->SetText(FText::AsNumber(Count));
	}

	// 选中高亮
	if (HighlightImage)
	{
		if (bHighlighted)
		{
			HighlightImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			HighlightImage->SetColorAndOpacity(HighlightColor);
		}
		else
		{
			HighlightImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}
