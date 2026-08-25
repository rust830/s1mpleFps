// Fill out your copyright notice in the Description page of Project Settings.


#include "MapBoundsActor.h"
#include "Components/BoxComponent.h"

AMapBoundsActor::AMapBoundsActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Bounds->SetBoxExtent(FVector(2000.f, 2000.f, 500.f));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetHiddenInGame(true); // 仅编辑器里用来框定，游戏里不渲染
	RootComponent = Bounds;
}

FVector2D AMapBoundsActor::GetMapMin() const
{
	const FVector Origin = Bounds->Bounds.Origin;
	const FVector Extent = Bounds->Bounds.BoxExtent;
	return FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y);
}

FVector2D AMapBoundsActor::GetMapMax() const
{
	const FVector Origin = Bounds->Bounds.Origin;
	const FVector Extent = Bounds->Bounds.BoxExtent;
	return FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y);
}
