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
	float MuzzleShakeIntensity = 1.5f;       // 每发子弹的震动强度

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	float MuzzleShakeDecaySpeed = 12.0f;     // 衰减速度，越大恢复越快

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakePitchRange = FVector2D(-1.5f, 0.5f);  // 俯仰抖动范围

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakeYawRange = FVector2D(-0.3f, 0.3f);    // 偏航抖动范围

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakeXRange = FVector2D(-0.3f, 0.3f);      // X 方向位移范围(cm)

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MuzzleShake")
	FVector2D MuzzleShakeZRange = FVector2D(-0.5f, 0.1f);      // Z 方向位移范围(cm)
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

	// --- 视角 / 机瞄(ADS) FOV 参数 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float DefaultFOV = 90.0f;                 // 腰射 FOV
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSFOV = 55.0f;                     // 开镜 FOV
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float AimInterpSpeed = 10.0f;             // FOV 插值速度
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSConcentration = 0.3f;            // 开镜时扩散系数（乘以 BaseSpread）

	// --- 第三人称 ADS 弹簧臂参数（暂用） ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSSpringArmLength = 60.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	FVector ADSSocketOffset = FVector(45.0f, 25.0f, 75.0f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	FVector ADSTargetOffset = FVector(-20.0f, 0.0f, 70.0f);

	// --- 机瞄对齐插槽名 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming|Sockets")
	FName AimCenterSocketName = "AimCenter";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming|Sockets")
	FName MuzzleSocketName = "muzzle";
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming|Sockets")
	FName SightAlignSocketName = FName(TEXT("sightalign"));

	// --- 机瞄几何对齐参数 ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSSightDistance = 150.0f;          // 瞄准时照门距相机前向的距离(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSSightVerticalOffset = 0.0f;      // 瞄准时照门相对眼睛的垂直偏移(cm)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Aiming")
	float ADSTransformInterpSpeed = 15.0f;    // 武器变换进入/退出 ADS 的插值速度
};
