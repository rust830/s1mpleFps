// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Blueprint/UserWidget.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "TimerManager.h"
#include "s1mpleFpsCharacter.generated.h"



class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UAnimMontage;
class UDamageComponent;
struct FInputActionValue;
class UArmorData;
class UHealthData;
class UGrenadeComponent;
class UHealthComponent;
class UWeaponInventoryComponent;
class UParticleSystem;
class USoundBase;


DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)	
class As1mpleFpsCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* ThirdPersonSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UDamageComponent* DamageComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UGrenadeComponent* GrenadeComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UHealthComponent* HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	UWeaponInventoryComponent* WeaponInventoryComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionStimuliSourceComponent* StimuliSource;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category=Input,meta=(AllowPrivateAccess="true"))
	UInputAction* Interaction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ToggleViewAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* GrenadeThrowAction;
	void OnThrowGrenade(const FInputActionValue& Value);

public:
	As1mpleFpsCharacter();

	FVector DefaultMeshRelativeLocation;
	FRotator DefaultMeshRelativeRotation;

	UPROPERTY(EditAnywhere, BluePrintReadOnly, Category = Input)
	UInputAction* PauseAction;

	// 嘲讽输入
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* TauntAction;


protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

public:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	void PauseGame();
	UFUNCTION(BlueprintImplementableEvent)
	void ShowHitMarket(bool bIsEnemy);

	// 命中标记：服务器 → 开枪者自己的客户端（参照 ClientDamageFeedback），交给 HUD 显示
	UFUNCTION(Client, Reliable)
	void PlayHitMarker(bool bIsEnemy);

	void Interact();

	void StartCrouch() {
		Crouch();
	}
	void EndCrouch() {
		UnCrouch();
	}

	void Reload();

public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="UI")
	TSubclassOf<class UUserWidget> HUDWidgetClass;
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="UI")
	class UUserWidget* HUDWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<class UUserWidget> DeathScreenWidgetClass;

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Audio")
	USoundBase* FootStepSound;
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	bool IsDead() const;

	void NextWeapon();
	void PreviousWeapon();

	// 武器槽 Action，在蓝图中分配按键
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WeaponSlot1Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WeaponSlot2Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* WeaponSlot3Action;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* NextWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* PrevWeaponAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* UseHealthAction;

	// 按键绑定回调
	void OnWeaponSlot1();
	void OnWeaponSlot2();
	void OnWeaponSlot3();
	void OnUseHealth();

	//ThirdPersonView
	UPROPERTY(BlueprintReadOnly)
	bool bIsThirdPerson = false;
	float LastViewToggleTime = -1.0f;
	float LastFootstepTime = 0.0f;
	UFUNCTION(BlueprintCallable)
	void ToggleView();
	// 第三人称武器挂点（默认用右手）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson")
	FName ThirdPersonWeaponSocket = TEXT("hand_r");
	// 第三人称时是否对其他玩家隐藏武器
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson")
	bool bHideWeaponFromOthers = true;
	void ReattachWeaponsForView(bool bToThirdPerson);

	// ===================== 第三人称动画（Montage 资产在 BP 里指定） =====================
	// 开火：高频，走 Unreliable multicast；建议放 UpperBody 槽，不打断腿部移动
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonFireMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonReloadMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonDeathMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonDeathMontageBackward;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonHitReactMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonRespawnMontage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	UAnimMontage* ThirdPersonIntroMontage; // 热身登场
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ThirdPerson|Animation")
	TArray<UAnimMontage*> ThirdPersonTauntMontages;

	// 在 GetMesh() 的 AnimInstance 上播一次 montage（本地）
	void PlayThirdPersonMontage(UAnimMontage* Montage);

	// 开火：NetMulticast + Unreliable（高频，丢包无所谓）。montage 作为参数传入——字段不复制，客户端拿不到
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastThirdPersonFire(UAnimMontage* FireMontage);

	// 枪口火焰/枪声：服务器广播给其他端（本地玩家在 StartSingleFire 已直接 spawn）。
	// 放角色身上，因为武器组件是动态建、RPC 不可靠（Invalid Net GUID）。
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastMuzzleFlash(FVector SpawnLocation, FRotator SpawnRotation, UParticleSystem* MuzzleFlash, USoundBase* FireSound);

	// 通用：NetMulticast + Reliable（换弹/重生/受击/登场/嘲讽等低频一次性动画）。montage 作为参数传入
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayThirdPersonMontage(UAnimMontage* Montage);

	// 死亡：NetMulticast + Reliable，播完自动隐藏第三人称 mesh（长度从参数计算，客户端无需复制字段）
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayDeathMontage(UAnimMontage* DeathMontage);

	// 死亡动画播完隐藏 mesh 的定时器（供 RespawnVisuals 清除，防止复活后被隐藏）
	FTimerHandle DeathHideTimerHandle;

	// 嘲讽：客户端→服务器→广播；服务器轮换选择 montage
	void OnTaunt();
	UFUNCTION(Server, Reliable)
	void ServerTaunt();

	// 嘲讽轮换状态（仅服务器有意义）
	int32 LastTauntIndex = -1;
	float LastTauntTime = 0.0f;

	void GrantArmor(UArmorData* ArmorData);
	
	void GrantHealthItem(UHealthData* HealthData);

	// 闪光弹效果
	UFUNCTION(Client, Reliable)
	void ClientApplyFlash(float Intensity, float Duration);

	// 受击屏幕血反馈（红色闪屏），服务器→客户端
	UFUNCTION(Client, Reliable)
	void ClientDamageFeedback(float Intensity, float Duration);

};
