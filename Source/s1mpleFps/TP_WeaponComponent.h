// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "WeaponData.h"
#include "Net/UnrealNetwork.h"
#include "TP_WeaponComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAmmoChanged, int32, CurrentAmmo, int32, SpareAmmo);

class As1mpleFpsCharacter;
class UWeaponData;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class S1MPLEFPS_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class As1mpleFpsProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	USoundBase* FireSound;

	/** Muzzle flash particle to spawn each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	UParticleSystem* MuzzleFlashEffect;

	/** AnimSequence to play each time we fire (played on the weapon's own skeletal mesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimSequence* FireAnimation;

	/** AnimSequence to play when reloading (played on the weapon's own skeletal mesh) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimSequence* ReloadAnimation;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector MuzzleOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFireModeEnum CurrentFireMode;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AimAction;

	/** Sets default values for this component's properties */
	UTP_WeaponComponent();

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool AttachWeapon(As1mpleFpsCharacter* TargetCharacter);

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	void SwitchFireMode();


	//WeaponData
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_WeaponData, Category="WeaponData")
	UWeaponData* WeaponData;
	UFUNCTION()
	void OnRep_WeaponData();
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentAmmo;
	UPROPERTY(BlueprintReadOnly)
	int32 SpareAmmo = 0;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentAmmo)
	int32 ReplicatedCurrentAmmo;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_SpareAmmo)
	int32 ReplicatedSpareAmmo;
	UPROPERTY(BlueprintAssignable)
	FOnAmmoChanged OnAmmoChanged;
	UFUNCTION()
	void OnRep_CurrentAmmo();
	UFUNCTION()
	void OnRep_SpareAmmo();
	UFUNCTION(Server,Reliable)
	void ServerReload();

	//Register
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsTriggerHeld = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 BurstRemaining = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTimerHandle AutoFireHandle;

	//Recoil And Shake
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D AccumulatedRecoil = FVector2D::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentSpread = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MuzzleShakeOffset = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator MuzzleShakeRotation = FRotator::ZeroRotator;
	// �����ҽ�ʱ��ԭʼ��Ա任������ÿ֡������أ�
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MuzzleOriginalLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator MuzzleOriginalRotation = FRotator::ZeroRotator;

	//Aimimg
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAiming=false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEquipped=false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ADSConcentration = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float AimInterpSpeed = 10.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentFOV = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetFOV = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ADSFOV = 55.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultFOV = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float ADSSpringArmLength = 150.0f;
	// ADS 时弹簧臂的摄像机偏移（过肩视角：偏右 + 偏高）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	FVector ADSSocketOffset = FVector(45.0f, 25.0f, 75.0f);
	// ADS 时摄像机看向的偏移点（视线补偿：向左拉让准心对准枪口）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	FVector ADSTargetOffset = FVector(-20.0f, 0.0f, 70.0f);
	float SavedSpringArmLength;
	FVector SavedSocketOffset = FVector::ZeroVector;
	FVector SavedTargetOffset = FVector::ZeroVector;

	// --- 机瞄(ADS)对齐参数：用枪上 muzzle / sightalign 插槽把机瞄直线对准准心 ---
	// 照门插槽名（枪骨骼体上）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	FName SightAlignSocketName = "sightalign";
	// 枪口插槽名（枪骨骼体上）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	FName MuzzleSocketName = "muzzle";
	// 瞄准时照门距相机前向的距离(cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float ADSSightDistance = 30.0f;
	// 瞄准时照门相对眼睛的垂直偏移(cm)，负值 = 略低于视线
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float ADSSightVerticalOffset = 0.0f;
	// 武器变换进入/退出 ADS 的插值速度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aiming")
	float ADSTransformInterpSpeed = 15.0f;
	// 当前 ADS 混合系数(0=腰射, 1=完全机瞄)
	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	float ADSBlendAlpha = 0.0f;
	// 缓存的机瞄相对变换（StartAiming 时算一次，之后每帧只插值，避免每帧重算导致乱转）
	FTransform CachedADSRelativeTransform = FTransform::Identity;
	bool bHasValidADSTransform = false;

	//Aiming Function
	UFUNCTION(BlueprintCallable)
	void StartAiming();
	UFUNCTION(BlueprintCallable)
	void EndAiming();
	UFUNCTION(BlueprintCallable)
	void ToggleAiming();

	// 计算把机瞄对准相机准心时、武器相对 GripPoint 的目标变换
	FTransform ComputeADSRelativeTransform() const;
	// 每帧混合腰射/机瞄变换
	void UpdateWeaponAim(float DeltaTime);

	//switch guns and equipped guns
	UFUNCTION(BlueprintCallable)
	void Equip();
	UFUNCTION(BlueprintCallable)
	void UnEquip();
	void SetOwningCharacter(As1mpleFpsCharacter* InChar) { Character = InChar; }

	//reload 
	UFUNCTION(BlueprintCallable)
	void Reload();
	UFUNCTION(BlueprintCallable)
	bool bCanFire();
	UFUNCTION(BlueprintCallable)
	void CancelReload();

	//FireMode
	UFUNCTION(BlueprintCallable)
	void StartSingleFire();
	UFUNCTION(BlueprintCallable)
	void StopAutoFire();

	UFUNCTION(Server,Reliable)
	void ServerFire(FVector SpawnLocation, FRotator SpawnRotation);
	UFUNCTION(NetMulticast,Reliable)
	void MulticastFireEffect(FVector SpawnLocation, FRotator SpawnRotation);
	void PerformFire(FVector SpawnLocation, FRotator SpawnRotation);




	UFUNCTION(BlueprintCallable)
	void StartFire();
	UFUNCTION(BlueprintCallable)
	void StopFire();

	/** Play an animation sequence on the weapon's own skeletal mesh (single-node mode, no AnimBP needed). */
	void PlayWeaponAnimation(UAnimSequence* Animation, bool bLooping = false);

	/** Apply WeaponData's mesh rotation/offset to relative transform. Call after AttachToComponent (SnapToTarget zeroes it). */
	void ApplyWeaponMeshTransform();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	UFUNCTION(BlueprintCallable)
	void ApplyAndDecayRecoil(float DeltaTime);
	UFUNCTION(BlueprintCallable)
	void ApplyAndDecaySpread(float DeltaTime);
	UFUNCTION(BlueprintCallable)
	void DecayMuzzleShake(float DeltaTime);



    

private:
	/** Fire rate cooldown timer */
	FTimerHandle FireRateTimerHandle;
	bool bIsOnFireCooldown = false;
	void ResetFireCooldown();

	/** Reload timer */
	FTimerHandle ReloadTimerHandle;
	bool bIsReloading = false;
	int32 PendingReloadAmount = 0;
	void FinishReload();

protected:
	/** Ends gameplay for this component. */
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** The Character holding this weapon*/
	UPROPERTY()
	As1mpleFpsCharacter* Character;
};
