// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/VerticalBox.h"
#include "Components/WrapBox.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"
#include "s1mpleFpsPlayerState.h"
#include "LobbyWidget.generated.h"

class UHeroEntryWidget;

/**
 * 大厅 UI 基类：把已选队的玩家名字分成蓝/红两列显示在两侧。
 *
 * 用法：
 *   1. 打开 WBP_Lobby，把它的父类（Class Settings -> Parent Class）改成 ULobbyWidget；
 *   2. 在画布里加两个 VerticalBox，分别命名为 BlueTeamList / RedTeamList，
 *      拖到左右两侧（左侧蓝队、右侧红队）。
 *
 * 数据来源：直接读已复制的 GameState->PlayerArray（PlayerName / Team / bReady 都已复制），
 * 定时轻量刷新即可，无需额外 RPC。
 */
UCLASS()
class S1MPLEFPS_API ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 蓝队名单（BindWidget，名字必须和蓝图一致）
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* BlueTeamList;

	// 红队名单（BindWidget，名字必须和蓝图一致）
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* RedTeamList;

	// 英雄选择容器（Wrap Box，C++ 自动铺卡片）
	UPROPERTY(meta = (BindWidget))
	UWrapBox* HeroList;

	// 英雄卡片类（WBP_HeroEntry，父类 UHeroEntryWidget）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hero")
	TSubclassOf<UHeroEntryWidget> HeroEntryClass;

	// 刷新间隔（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	float RefreshInterval = 0.25f;

	// 是否在已就绪的玩家名字后加「✓」
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	bool bShowReadyMark = true;

	// 两列名字颜色
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	FLinearColor BlueColor = FLinearColor(0.25f, 0.6f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	FLinearColor RedColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lobby")
	int32 NameFontSize = 18;

	// 手动触发一次刷新（蓝图也能调）
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void RefreshTeamLists();

	// 按 HeroRoster 铺英雄卡片（首帧自动调一次）
	UFUNCTION(BlueprintCallable, Category = "Hero")
	void RefreshHeroList();

	// 刷新选中高亮（定时调，随 SelectedHeroIndex 变化）
	void RefreshHeroHighlight();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void AddNameRow(UVerticalBox* Box, const FString& RowText, const FLinearColor& Color);

	void OnRefreshTimer();

	// 缓存已渲染的名单，用于避免每帧无谓重建 UWidget（防闪烁）
	TArray<FString> CachedBlueRows;
	TArray<FString> CachedRedRows;

	// 已生成的英雄卡片（供刷新高亮时遍历）
	TArray<UHeroEntryWidget*> HeroEntries;

	FTimerHandle RefreshTimerHandle;

	// 英雄卡片是否已铺过一次
	bool bHeroListBuilt = false;
};
