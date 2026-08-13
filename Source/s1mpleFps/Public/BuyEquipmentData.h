// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponData.h"
#include "HealthData.h"
#include "ArmorData.h"
#include "GrenadeData.h"
#include "BuyEquipmentData.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMessageReceived, const FString&, Sender,const FString&, Message, bool, bIsTeam);
class UTP_WeaponComponent;
/**
 * 
 */
USTRUCT(BlueprintType)
struct FBuyItemEntry {
	GENERATED_BODY()
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 Price = 0;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName TypeName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UTP_WeaponComponent> WeaponClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UWeaponData* WeaponData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UArmorData* ArmorData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UHealthData* HealthData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGrenadeData* GrenadeData;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 GrenadeAmount = 1;
};
UCLASS()
class S1MPLEFPS_API UBuyEquipmentData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FBuyItemEntry> Items;
};
