// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyWidget.h"
#include "HeroEntryWidget.h"
#include "s1mpleFpsPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 立刻刷一遍队伍名单（UTextBlock 没有构造时序问题）
	RefreshTeamLists();

	// 用定时器驱动刷新——UUserWidget 默认不 Tick，靠 Timer 最稳。
	// 英雄卡片放到定时器第一发里铺：NativeConstruct 期间 add child，子 UUserWidget 的 NativeConstruct 可能不触发。
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			RefreshTimerHandle, this, &ULobbyWidget::OnRefreshTimer, RefreshInterval, true);
	}
}

void ULobbyWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}
	Super::NativeDestruct();
}

void ULobbyWidget::OnRefreshTimer()
{
	// 首发改铺英雄卡片（等父 Widget 完全构建好再 add child，子 NativeConstruct 才会触发）
	if (!bHeroListBuilt)
	{
		bHeroListBuilt = true;
		RefreshHeroList();
	}

	RefreshTeamLists();
	RefreshHeroHighlight();
}

void ULobbyWidget::RefreshTeamLists()
{
	if (!BlueTeamList || !RedTeamList)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AGameStateBase* GS = World->GetGameState<AGameStateBase>();
	if (!GS)
	{
		return;
	}

	// 先收集本轮名单（名字 + 就绪标记）
	TArray<FString> BlueRows;
	TArray<FString> RedRows;

	for (APlayerState* Entry : GS->PlayerArray)
	{
		As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Entry);
		if (!PS)
		{
			continue;
		}

		FString Row = PS->GetPlayerName();
		if (bShowReadyMark && PS->bReady)
		{
			Row += TEXT(" ✓");
		}

		if (PS->Team == ETeam::Blue)
		{
			BlueRows.Add(Row);
		}
		else if (PS->Team == ETeam::Red)
		{
			RedRows.Add(Row);
		}
		// Team == None 的玩家（还没选队）不显示
	}

	// 名单没变就跳过，避免每 0.25s 重建 UWidget
	if (BlueRows == CachedBlueRows && RedRows == CachedRedRows)
	{
		return;
	}
	CachedBlueRows = MoveTemp(BlueRows);
	CachedRedRows = MoveTemp(RedRows);

	BlueTeamList->ClearChildren();
	RedTeamList->ClearChildren();

	for (const FString& Row : CachedBlueRows)
	{
		AddNameRow(BlueTeamList, Row, BlueColor);
	}
	for (const FString& Row : CachedRedRows)
	{
		AddNameRow(RedTeamList, Row, RedColor);
	}
}

void ULobbyWidget::AddNameRow(UVerticalBox* Box, const FString& RowText, const FLinearColor& Color)
{
	if (!Box)
	{
		return;
	}

	UTextBlock* Row = NewObject<UTextBlock>(this);
	Row->SetText(FText::FromString(RowText));
	Row->SetJustification(ETextJustify::Center);
	Row->SetColorAndOpacity(FSlateColor(Color));

	FSlateFontInfo Font = Row->GetFont();
	Font.Size = NameFontSize;
	Font.OutlineSettings.OutlineSize = 1;
	Font.OutlineSettings.OutlineColor = FLinearColor::Black;
	Row->SetFont(Font);

	Box->AddChildToVerticalBox(Row);
}

void ULobbyWidget::RefreshHeroList()
{
	As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetOwningPlayer());

	if (!HeroList)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] 英雄列表没显示：WBP_Lobby 里没有名为 HeroList 的 Wrap Box"));
		return;
	}
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] 英雄列表没显示：拿不到 PlayerController"));
		return;
	}
	if (!HeroEntryClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] 英雄列表没显示：HeroEntryClass 没设（WBP_Lobby 默认值 → Hero → Hero Entry Class）"));
		return;
	}
	if (PC->HeroRoster.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Lobby] 英雄列表没显示：HeroRoster 为空（BP_FirstPersonPlayerController → HeroRoster）"));
		return;
	}

	HeroList->ClearChildren();
	HeroEntries.Empty();

	for (int32 i = 0; i < PC->HeroRoster.Num(); ++i)
	{
		UHeroEntryWidget* Entry = CreateWidget<UHeroEntryWidget>(this, HeroEntryClass);
		if (!Entry) continue;
		Entry->SetupHero(PC->HeroRoster[i], i);
		HeroList->AddChildToWrapBox(Entry);
		HeroEntries.Add(Entry);
	}

	UE_LOG(LogTemp, Log, TEXT("[Lobby] 英雄卡片已铺 %d 张"), HeroEntries.Num());
}

void ULobbyWidget::RefreshHeroHighlight()
{
	if (HeroEntries.Num() == 0) return;

	As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetOwningPlayer());
	if (!PC) return;
	As1mpleFpsPlayerState* PS = PC->GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS) return;

	for (UHeroEntryWidget* Entry : HeroEntries)
	{
		if (!Entry) continue;
		Entry->SetSelected(Entry->GetHeroIndex() == PS->SelectedHeroIndex);
	}
}
