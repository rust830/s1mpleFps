// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "HeroData.h"
#include "HeroEntryWidget.generated.h"

/**
 * 英雄选择卡片。一个卡片 = 一个英雄（头像 + 名字 + 点选 + 选中高亮）。
 *
 * 蓝图用法：建 WBP_HeroEntry，父类选 UHeroEntryWidget，层级：
 *   CardButton（根 Button）
 *     ├─ PortraitImage（头像 Image）
 *     ├─ NameText（名字 TextBlock）
 *     └─ HighlightImage（选中高亮 Image，默认 Collapsed，选中时显示高亮色）
 * 高亮图可以是纯色半透明块，也可以是一张边框贴图（会用 HighlightColor 染色）。
 */
UCLASS()
class S1MPLEFPS_API UHeroEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget))
	UButton* CardButton;

	UPROPERTY(meta = (BindWidget))
	UImage* PortraitImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NameText;

	// 选中高亮（可选：加一个 Image，选中时显示 + 染色）
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* HighlightImage;

	// 选中高亮颜色（只对 HighlightImage 生效）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero")
	FLinearColor HighlightColor = FLinearColor(1.0f, 0.8f, 0.1f, 0.65f);

	// 由 ULobbyWidget 调用：存下英雄 + 索引（实际套视觉在 NativeConstruct 里做，此时 BindWidget 已就位）
	void SetupHero(UHeroData* InHero, int32 InIndex);

	// 设置选中高亮
	void SetSelected(bool bSelected);

	int32 GetHeroIndex() const { return HeroIndex; }

protected:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(Transient)
	UHeroData* Hero = nullptr;

	int32 HeroIndex = 0;

	// 当前是否选中（用于脏检查，避免每 0.25s 重复设置导致闪烁）
	bool bIsSelected = false;

	UFUNCTION()
	void OnCardClicked();
};
