// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "s1mpleFpsPlayerState.h"
#include "MinimapWidget.generated.h"

class UImage;
class UCanvasPanel;
class UTexture2D;
class AControlArea;

// 小地图核心逻辑：把世界坐标映射到地图上，刷新自己/队友/占点点位图标。
// 蓝图继承它（WBP_Minimap），放好 MapImage（背景截图）和 IconLayer（图标层）即可。
UCLASS()
class S1MPLEFPS_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ---- 蓝图里放的控件 ----
	UPROPERTY(meta = (BindWidget))
	UImage* MapImage;

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* IconLayer;

	// ---- 图标纹理（蓝图里设） ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTexture2D* PlayerIcon;          // 自己
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTexture2D* TeammateIcon;        // 队友
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTexture2D* EnemyIcon;           // 敌人
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTexture2D* ZoneIcon;            // 占点位（未激活）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	UTexture2D* ZoneIconActive;      // 占点位（激活高亮）

	// ---- 尺寸 / 颜色 ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D MapSize = FVector2D(256.f, 256.f);   // 需与蓝图里 MapImage 渲染尺寸一致
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D IconSize = FVector2D(16.f, 16.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor SelfColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor TeammateColor = FLinearColor(0.25f, 0.6f, 1.0f);   // 蓝
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor EnemyColor = FLinearColor(1.0f, 0.25f, 0.25f);     // 红

	// 刷新间隔（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float RefreshInterval = 0.05f;

	// 世界坐标 → 小地图坐标（左上角原点，含中心对齐修正前的原始映射）
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	FVector2D WorldToMap(const FVector& WorldPos) const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	FVector2D MapMin = FVector2D::ZeroVector;
	FVector2D MapMax = FVector2D(1.f, 1.f);
	bool bHasBounds = false;

	void CacheMapBounds();
	void RefreshIcons();

	UImage* CreateIcon(UTexture2D* Texture, const FLinearColor& Color);
	void PositionIcon(UImage* Icon, const FVector& WorldPos);

	// 图标缓存（避免每帧创建/销毁）
	TMap<APlayerState*, UImage*> PlayerIconMap;
	TMap<AControlArea*, UImage*> ZoneIconMap;

	float RefreshTimer = 0.f;
};
