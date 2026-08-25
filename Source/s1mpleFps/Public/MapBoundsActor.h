// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MapBoundsActor.generated.h"

class UBoxComponent;

// 关卡里放一个，缩放到刚好框住「小地图要覆盖的可玩区域」。
// 小地图 Widget 读它的包围盒作为世界范围（Min/Max）；Box 之外的部分不会出现在地图上。
UCLASS()
class S1MPLEFPS_API AMapBoundsActor : public AActor
{
	GENERATED_BODY()

public:
	AMapBoundsActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Minimap")
	TObjectPtr<UBoxComponent> Bounds;

	// X/Y 平面的地图范围（世界坐标，忽略 Z）
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	FVector2D GetMapMin() const;

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	FVector2D GetMapMax() const;
};
