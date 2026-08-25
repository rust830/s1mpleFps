// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapWidget.h"
#include "Blueprint/WidgetTree.h"
#include "MapBoundsActor.h"
#include "ControlArea.h"
#include "EngineUtils.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/Texture2D.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	CacheMapBounds();
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshTimer += InDeltaTime;
	if (RefreshTimer >= RefreshInterval)
	{
		RefreshTimer = 0.f;
		RefreshIcons();
	}
}

void UMinimapWidget::CacheMapBounds()
{
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<AMapBoundsActor> It(World); It; ++It)
	{
		MapMin = It->GetMapMin();
		MapMax = It->GetMapMax();
		bHasBounds = true;
		return;
	}
}

FVector2D UMinimapWidget::WorldToMap(const FVector& WorldPos) const
{
	if (!bHasBounds || MapMax.X == MapMin.X || MapMax.Y == MapMin.Y)
	{
		return FVector2D::ZeroVector;
	}
	const float NX = (WorldPos.X - MapMin.X) / (MapMax.X - MapMin.X);
	const float NY = (WorldPos.Y - MapMin.Y) / (MapMax.Y - MapMin.Y);
	return FVector2D(NX * MapSize.X, (1.0f - NY) * MapSize.Y);
}

UImage* UMinimapWidget::CreateIcon(UTexture2D* Texture, const FLinearColor& Color)
{
	if (!IconLayer) return nullptr;

	UImage* Img = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	if (Texture) Img->SetBrushFromTexture(Texture);
	Img->SetColorAndOpacity(Color);

	IconLayer->AddChild(Img);
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Img->Slot))
	{
		CanvasSlot->SetSize(IconSize);
		CanvasSlot->SetAutoSize(false);
	}
	return Img;
}

void UMinimapWidget::PositionIcon(UImage* Icon, const FVector& WorldPos)
{
	if (!Icon) return;
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Icon->Slot))
	{
		// 减去图标半尺寸，让图标中心对齐到映射点
		CanvasSlot->SetPosition(WorldToMap(WorldPos) - IconSize * 0.5f);
	}
}

void UMinimapWidget::RefreshIcons()
{
	if (!bHasBounds || !IconLayer) return;
	UWorld* World = GetWorld();
	if (!World) return;

	AGameStateBase* GS = World->GetGameState<AGameStateBase>();
	APlayerController* LocalPC = GetOwningPlayer();
	As1mpleFpsPlayerState* MyPS = LocalPC ? LocalPC->GetPlayerState<As1mpleFpsPlayerState>() : nullptr;
	const ETeam MyTeam = MyPS ? MyPS->Team : ETeam::None;

	// === 玩家图标（自己 / 队友 / 敌人） ===
	TSet<APlayerState*> SeenPlayers;
	if (GS)
	{
		for (APlayerState* PS : GS->PlayerArray)
		{
			As1mpleFpsPlayerState* PPS = Cast<As1mpleFpsPlayerState>(PS);
			if (!PPS) continue;
			APawn* Pawn = PS->GetPawn();
			if (!Pawn) continue;
			SeenPlayers.Add(PS);

			const bool bSelf = (PS == MyPS);
			const bool bTeammate = (!bSelf && MyTeam != ETeam::None && PPS->Team == MyTeam);
			UTexture2D* Tex = bSelf ? PlayerIcon : (bTeammate ? TeammateIcon : EnemyIcon);
			const FLinearColor Color = bSelf ? SelfColor : (bTeammate ? TeammateColor : EnemyColor);

			UImage* Icon = nullptr;
			if (PlayerIconMap.Contains(PS))
			{
				Icon = PlayerIconMap[PS];
			}
			else
			{
				Icon = CreateIcon(Tex, Color);
				if (Icon) PlayerIconMap.Add(PS, Icon);
			}

			if (Icon)
			{
				if (Tex) Icon->SetBrushFromTexture(Tex);
				Icon->SetColorAndOpacity(Color);
				PositionIcon(Icon, Pawn->GetActorLocation());
			}
		}
	}

	// 移除消失的玩家图标
	TArray<APlayerState*> ToRemovePlayers;
	for (const auto& Pair : PlayerIconMap)
	{
		if (!SeenPlayers.Contains(Pair.Key)) ToRemovePlayers.Add(Pair.Key);
	}
	for (APlayerState* PS : ToRemovePlayers)
	{
		if (UImage* Icon = PlayerIconMap[PS]) Icon->RemoveFromParent();
		PlayerIconMap.Remove(PS);
	}

	// === 占点点位图标（激活高亮） ===
	TSet<AControlArea*> SeenZones;
	for (TActorIterator<AControlArea> It(World); It; ++It)
	{
		AControlArea* Area = *It;
		SeenZones.Add(Area);

		UImage* Icon = nullptr;
		if (ZoneIconMap.Contains(Area))
		{
			Icon = ZoneIconMap[Area];
		}
		else
		{
			Icon = CreateIcon(ZoneIcon, FLinearColor::White);
			if (Icon) ZoneIconMap.Add(Area, Icon);
		}

		if (Icon)
		{
			const bool bActive = Area->bActive;
			Icon->SetBrushFromTexture(bActive ? ZoneIconActive : ZoneIcon);
			Icon->SetColorAndOpacity(bActive ? FLinearColor::White : FLinearColor(1.f, 1.f, 1.f, 0.4f));
			PositionIcon(Icon, Area->GetActorLocation());
		}
	}

	// 移除消失的点位图标
	TArray<AControlArea*> ToRemoveZones;
	for (const auto& Pair : ZoneIconMap)
	{
		if (!SeenZones.Contains(Pair.Key)) ToRemoveZones.Add(Pair.Key);
	}
	for (AControlArea* Area : ToRemoveZones)
	{
		if (UImage* Icon = ZoneIconMap[Area]) Icon->RemoveFromParent();
		ZoneIconMap.Remove(Area);
	}
}
