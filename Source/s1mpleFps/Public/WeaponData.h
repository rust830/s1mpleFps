// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/SkeletalMesh.h"
#include "Particles/ParticleSystem.h"
#include "Animation/AnimSequence.h"
#include "InputAction.h"
#include "InputMappingContext.h"

#include "WeaponData.generated.h"

UENUM(BlueprintType)
enum class EFireModeEnum : uint8
{
	SemiAuto   UMETA(DisplayName = "Semi-Auto"),
	Burst      UMETA(DisplayName = "Burst 3-Round"),
	FullAuto   UMETA(DisplayName = "Full-Auto")
};
class As1mpleFpsProjectile;
/**
 *
 */

UCLASS()
class S1MPLEFPS_API UWeaponData : public UPrimaryDataAsset
{
public:

	GENERATED_BODY()


	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="WeaponData")
	float BaseDamage = 25.0f;

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="WeaponData")
	int32 MaxProjectile = 30;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	int32 TotalProjectiles = 100;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category="WeaponData")
	float MaxDistance = 10000.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly,Category="WeaponData")
	FString WeaponName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float FireRate = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float ReloadTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	UParticleSystem* MuzzleFlashEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	UAnimSequence* FireAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	UAnimSequence* ReloadAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	TSubclassOf<As1mpleFpsProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float ArmorPenetration = 0.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="WeaponData")
	EFireModeEnum FireMode = EFireModeEnum::FullAuto;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
	int32 BurstCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WeaponData")
	bool bCanSwitchFireMode = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float MinVertical = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float MaxVertical = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float MinHorizon = -0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float MaxHorizon = 0.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	float RecoilRecoverySpeed = 15.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Spread")
	float BaseSpread = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Spread")
	float MaxSpread = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Spread")
	float SpreadPerShot = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Recoil|Spread")
	float SpreadRecoverySpeed = 6.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	float MuzzleShakeIntensity = 1.5f;       // ÿ���������ǿ��

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	float MuzzleShakeDecaySpeed = 12.0f;     // ˥���ٶȣ�Խ�����Խ�죩

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakePitchRange = FVector2D(-1.5f, 0.5f);  // ���������Χ

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakeYawRange = FVector2D(-0.3f, 0.3f);    // ƫ�������Χ

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakeXRange = FVector2D(-0.3f, 0.3f);      // ����λ�Ʒ�Χ(cm)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakeZRange = FVector2D(-0.5f, 0.1f);      // ����λ�Ʒ�Χ(cm)
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float OptimalCombatDistance = 5000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	USkeletalMesh* WeaponMesh;

	// 武器挂到 GripPoint 后的相对旋转（不同网格轴向不同：AK-47 枪管沿 Y 需旋转，标准枪沿 X 则为 0）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	FRotator WeaponMeshRotation = FRotator::ZeroRotator;

	// 武器挂到 GripPoint 后的相对位移（对齐握把）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData")
	FVector WeaponMeshOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData|Input")
	UInputMappingContext* WeaponMappingContext;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData|Input")
	UInputAction* WeaponFireAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData|Input")
	UInputAction* WeaponReloadAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData|Input")
	UInputAction* WeaponSwitchAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "WeaponData|Input")
	UInputAction* WeaponAimAction;
};
