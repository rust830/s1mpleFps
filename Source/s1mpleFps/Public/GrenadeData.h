// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GrenadeData.generated.h"


/**
 * 
 */
UENUM(BlueprintType)
enum class EGrenadeType : uint8 {
	Frag UMETA(DisplayName = "Frag Grenade"),
	Flash UMETA(DisplayName = "Flashbang"),
	Smoke UMETA(DisplayName = "Smoke Grenade"),
};
UCLASS()
class S1MPLEFPS_API UGrenadeData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="GrenadeType")
	EGrenadeType GrenadeType = EGrenadeType::Frag;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FusingTime = 5.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = " GrenadeType == EGrenadeType::Frag"))
	float ExplodeRadius = 300.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = " GrenadeType == EGrenadeType::Frag"))
	float MaxDamage = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Explosion", meta = (EditCondition = "GrenadeType == EGrenadeType::Frag"))
	float Penetration = 0;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash", meta = (EditCondition = "GrenadeType == EGrenadeType::Flash"))
	float FlashRadius = 200.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash", meta = (EditCondition = " GrenadeType == EGrenadeType::Flash"))
	float FlashDuration = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash", meta = (EditCondition = " GrenadeType == EGrenadeType::Flash"))
	float FlashIntensityScale = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Flash", meta = (EditCondition = " GrenadeType == EGrenadeType::Flash"))
	float DeafenTime = 3.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (EditCondition = "GrenadeType == EGrenadeType::Smoke"))
	float SmokeRadius = 500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Smoke", meta = (EditCondition = "GrenadeType == EGrenadeType::Smoke"))
	float SmokeDuration = 12.f;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bAllowedCooking = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="Throw")
	float HighThrowSpeed = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw")
	float HighThrowAngle = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw")
	float LowThrowSpeed = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw")
	float LowThrowAngle = 10.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw")
	float GravityScale= 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw")
	float Bounciness = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Throw")
	float ThrowCooldown = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	UStaticMesh* GrenadeMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundBase* ExplosionSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundBase* BounceSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundBase* EquipSound = nullptr;     

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundBase* PinPullSound = nullptr;   

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
	USoundBase* CookingTickSound = nullptr; 

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	UParticleSystem* ExplosionEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
	UParticleSystem* TrailEffect = nullptr;   
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	UTexture2D* GrenadeIcon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FString GrenadeName = TEXT("Frag Grenade");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<class AGrenadeProjectile> GrenadeProjectileClass;
};
