// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"	
#include "Engine/DataAsset.h"
#include "WeaponDataAsset.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FHitZoneEntry {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FName BoneName;
	UPROPERTY(EditAnywhere,BlueprintReadonly)
	float DamageMul = 1.0f;
};

UCLASS()
class S1MPLEFPS_API UWeaponDataAsset : public UPrimaryDataAsset
{
public:
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TArray<FHitZoneEntry> HitZones;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float Multiplier = 1.0f;//default body;
	float GetMul(FName BoneName) {
		for (FHitZoneEntry& Entry : HitZones) {
			if (BoneName == Entry.BoneName) {
				return Entry.DamageMul;
			}
		
		}
		return Multiplier;
	}
};
