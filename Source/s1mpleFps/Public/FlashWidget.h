// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Widgets/SWidget.h"
#include "FlashWidget.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API UFlashWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Color 默认白色（闪光弹），受击血反馈传入红色
	void StartFlashing(float Intensity, float Duration, FLinearColor Color = FLinearColor::White);
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime)override;

	UPROPERTY()
	UImage* Image;
private:
	void BuildFlashUI();

	bool bIsFlashing = false;
	float CurrentAlpha;
	float AlphaPerSecond;
	FLinearColor FlashColor = FLinearColor::White;
};
