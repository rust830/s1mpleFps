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
class UHealthComponent;


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

	// --- Network replication ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

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

};
