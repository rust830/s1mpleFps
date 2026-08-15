// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "TimerManager.h"
#include "Blueprint/UserWidget.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Net/UnrealNetwork.h"
#include "s1mpleFpsCharacter.generated.h"



class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UInputMappingContext;
class UDamageComponent;
struct FInputActionValue;
class UWeaponData;
class UArmorData;
class UHealthData;
class UTP_WeaponComponent;
class UGrenadeComponent;


DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthItemsChanged);

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
	UFUNCTION()
	void Die();
	UFUNCTION()
	void OnHealthDamaged(float Damage, AActor* DamageInstigator);
	UFUNCTION(BlueprintCallable)
	void Respawn();
	void RespawnVisuals();
	UFUNCTION(BlueprintCallable)
	void HideDeathWidget();
	bool bIsDead = false;

	// --- Network replication ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnHealthItemsChanged OnHealthItemsChanged;
	UPROPERTY(BlueprintAssignable)
	FOnHealthItemsChanged OnHealingStateChanged;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
	float ReplicatedHealth = 100.0f;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bIsDead)
	bool bIsDeadReplicated = false;
	UFUNCTION()
	void OnRep_Health();
	UFUNCTION()
	void OnRep_bIsDead();
	UFUNCTION(Server, Reliable)
	void ServerRequestRespawn();

	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Respawn")
	float RespawnDelay = 5.0f;
	FTimerHandle RespawnHandle;

	FVector DefaultMeshRelativeLocation;
	FRotator DefaultMeshRelativeRotation;

	UPROPERTY(EditAnywhere, BluePrintReadOnly, Category = Input)
	UInputAction* PauseAction;


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

	void PlayHitMarker(bool bIsEnemy);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float PickUpDistance = 300.0f;
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

	UPROPERTY(BlueprintReadOnly, Category = "UI")
	class UUserWidget* DeathScreenWidget;

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

	UFUNCTION(Server,Reliable)
	void ServerPickUpWeapon(AActor* HitActor);
	UFUNCTION(Client,Reliable)
	void ClientUndoPickUp(AActor* HitActor);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPickUp(AActor* HitActor);
	UFUNCTION(Server,Reliable)
	void ServerSwitchWeapon(int32 Index);
	UFUNCTION(Server,Reliable)
	void ServerPreviousWeapon();
	UFUNCTION(Server,Reliable)
	void ServerNextWeapon();
	UFUNCTION(Client,Reliable)
	void ClientSyncWeaponAmmo(int32 InWeaponIndex, int32 InCurrentAmmo, int32 InSpareAmmo);
	UFUNCTION(Server,Reliable)
	void ServerFireWeapon(int32 InWeaponIndex, FVector SpawnLocation, FRotator SpawnRotation);
	UFUNCTION(Server,Reliable)
	void ServerReloadWeapon(int32 InWeaponIndex);

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_CurrentWeapon)
	UTP_WeaponComponent* CurrentWeapon;
	UFUNCTION()
	void OnRep_CurrentWeapon();
	void SetActiveWeapon(UTP_WeaponComponent* NewWeapon);
	UPROPERTY()
	UTP_WeaponComponent* PreviousClientWeapon = nullptr;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing=OnRep_WeaponInventory)
	TArray<UTP_WeaponComponent*> WeaponInventory;
	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 WeaponIndex = 0;
	UFUNCTION()
	void OnRep_WeaponInventory();
	UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Weapon")
	int32 MaxWeaponSlots = 3;
	void SwitchWeapon(int32 Index);
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
	int32 GrantWeapon(TSubclassOf<UTP_WeaponComponent> WeaponClass, UWeaponData* WeaponDataPtr = nullptr);
	int32 PendingPurchaseIndex = -1;
	FTimerHandle PurchaseRetryHandle;
	int32 PurchaseRetryCount = 0;
	void RemoveWeaponSlot(int32 RemoveIndex);

	void GrantArmor(UArmorData* ArmorData);
	
	void GrantHealthItem(UHealthData* HealthData);

	// 闪光弹效果
	UFUNCTION(Client, Reliable)
	void ClientApplyFlash(float Intensity, float Duration);

	// 受击屏幕血反馈（红色闪屏），服务器→客户端
	UFUNCTION(Client, Reliable)
	void ClientDamageFeedback(float Intensity, float Duration);

	UPROPERTY(ReplicatedUsing=OnRep_HealthItems,BlueprintReadWrite)
	TArray<UHealthData*> HealthTypes;
	UPROPERTY(ReplicatedUsing=OnRep_HealthItems,BlueprintReadWrite)
	TArray<int32> HealthAmount;

	UFUNCTION()
	void OnRep_HealthItems();
	UFUNCTION(Server,Reliable)
	void ServerUseHealth(int32 HealthIndex);
	UFUNCTION(BlueprintCallable)
	void UseHealth(int32 HealthIndex);
	// 取消打药：恢复移速、清定时器、收起进度环（本地清理，客户端/服务器各自调用）
	UFUNCTION(BlueprintCallable)
	void CancelHealing();
	// 客户端通知服务器取消打药（否则服务器定时器仍会结算回血+扣道具）
	UFUNCTION(Server, Reliable)
	void ServerCancelHealing();
	// 服务器通知客户端取消打药（受击打断时让客户端本地 UI/移速也复位）
	UFUNCTION(Client, Reliable)
	void ClientCancelHealing();
	UPROPERTY(BlueprintReadOnly)
	bool bIsHealing = false;
	UPROPERTY(BlueprintReadOnly)
	float HealingDuration = 0.0f;
	FTimerHandle HealingHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
	float HealingSpeedMultiplier = 0.5f;

private:
	float SavedWalkSpeed;

};
