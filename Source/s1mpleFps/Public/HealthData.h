// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HealthData.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API UHealthData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Medicine")
	float HealAmount = 30.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medicine")
	float UsingTime = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medicine")
	int32 price = 20;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Medicine")
	FName MedicineName;
};
