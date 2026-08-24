// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "GrenadeSlotEntry.generated.h"

class UGrenadeData;

/**
 * 手雷 HUD 的单个槽位。一个槽 = 一种手雷（图标 + 数量 + 选中高亮）。
 *
 * 蓝图用法：建 WBP_GrenadeSlot，父类选 UGrenadeSlotEntry，层级：
 *   GrenadeIconImage（图标 Image）
 *   GrenadeCountText（数量 TextBlock）
 *   HighlightImage（选中高亮 Image，可选，默认 Collapsed，选中时显示并染 HighlightColor）
 * 高亮图可以是纯色半透明块，也可以是一张边框贴图。
 *
 * 数据由 UHUDWidget::RefreshGrenadeSlots 在 CreateWidget 后、AddChild 前通过 SetupSlot 传入；
 * 真正的视觉套用在 NativeConstruct 里做（此时 BindWidget 已就位）。
 */
UCLASS()
class S1MPLEFPS_API UGrenadeSlotEntry : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UImage* GrenadeIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadeCountText;

	// 选中高亮（可选：加一个 Image，选中时显示 + 染色）
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HighlightImage;

	// 选中高亮颜色（只对 HighlightImage 生效）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	FLinearColor HighlightColor = FLinearColor(1.0f, 0.85f, 0.2f, 0.9f);

	// 图标渲染尺寸（像素）。SetBrushFromTexture 会把 brush 的 ImageSize 设成贴图原始尺寸（可能非常大），
	// 这里强制设成固定值，否则图标会以原始分辨率撑大、越界到屏幕外。
	// （HeroEntry 靠 Button 容器约束头像，同理；CanvasPanel 不会约束，必须显式设。）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	FVector2D IconSize = FVector2D(40.f, 40.f);

	// 由 UHUDWidget 调用：存下数据 + 是否选中（实际套视觉在 NativeConstruct 里做）
	void SetupSlot(UGrenadeData* InData, int32 InCount, bool bInHighlighted);

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	UGrenadeData* GrenadeData = nullptr;

	int32 Count = 0;

	// 当前是否选中（用于脏检查，避免重复设置导致闪烁）
	bool bHighlighted = false;
};
