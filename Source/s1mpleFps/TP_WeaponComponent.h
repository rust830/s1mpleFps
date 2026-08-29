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

// 本组件只负责「状态 + 动作」；所有配置数据（伤害/射速/音效/动画/输入/机瞄参数等）都在 UWeaponData 里。
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class S1MPLEFPS_API UTP_WeaponComponent : public USkeletalMeshComponent
{
	GENERATED_BODY()

public:

	/** 当前射击模式（运行时状态，初始值取 WeaponData->FireMode） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EFireModeEnum CurrentFireMode;

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
	// 开火时保存原始枪口位置/旋转，供每帧枪口震动复位
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector MuzzleOriginalLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator MuzzleOriginalRotation = FRotator::ZeroRotator;

	//Aiming（状态）
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsAiming=false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsEquipped=false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CurrentFOV = 90.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetFOV = 90.0f;
	float SavedSpringArmLength;
	FVector SavedSocketOffset = FVector::ZeroVector;
	FVector SavedTargetOffset = FVector::ZeroVector;

	// 当前 ADS 混合系数(0=腰射, 1=完全机瞄)
	UPROPERTY(BlueprintReadOnly, Category = "Aiming")
	float ADSBlendAlpha = 0.0f;
	// 机瞄插槽是否有效（StartAiming 校验一次；真正的目标变换在 UpdateWeaponAim 每帧重算）
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
	// 用握把/扳机骨骼（在枪管下方）自动确定枪的"上"轴，兜底 +Z
	FVector ComputeLocalUp(const FVector& AimAxis) const;
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
	UFUNCTION(NetMulticast, Unreliable)
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
