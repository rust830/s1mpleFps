// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ArmorData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FNameToReduction {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FName ArmorName;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float DamageReduction = 0.0f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TArray<FName> CoveredBones;
};
UCLASS()
class S1MPLEFPS_API UArmorData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FString ArmorName;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	TArray<FNameToReduction> Armor;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	float Reduction=0.0f;
	float GetReduction(FName HitBone) const {
		for (FNameToReduction Armors : Armor) {
			 
				if (Armors.CoveredBones.Contains(HitBone)) {
					return Armors.DamageReduction;
				}			
		}
		return Reduction;
	}
};
