// Copyright Epic Games, Inc. All Rights Reserved.

#include "s1mpleFpsCharacter.h"
#include "DamageComponent.h"
#include "HealthComponent.h"
#include "TP_PickUpComponent.h"
#include "TP_WeaponComponent.h"
#include "Engine/OverlapResult.h"
#include "s1mpleFpsProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "TimerManager.h"
#include "Engine/TimerHandle.h"
#include "Engine/LocalPlayer.h"
#include "WeaponData.h"
#include "HealthData.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "TP_WeaponComponent.h"
#include "EnemyAIController.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "s1mpleFpsPlayerController.h"
#include "HUDWidget.h"
#include "s1mpleFpsGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "s1mpleFpsPvPGameMode.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameState.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "FlashWidget.h"
#include "GrenadeComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// As1mpleFpsCharacter

void As1mpleFpsCharacter::OnThrowGrenade(const FInputActionValue& Value)
{
	if (GrenadeComponent)
		GrenadeComponent->ToggleGrenadeMode();
}


As1mpleFpsCharacter::As1mpleFpsCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	// 给第一人称手臂网格赋值（裸 C++ 实例也必须有 SkeletalMesh，否则 GripPoint 会报 "No SkeletalMesh"）
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh1PObj(TEXT("/Game/FirstPersonArms/Character/Mesh/SK_Mannequin_Arms"));
	if (Mesh1PObj.Succeeded())
	{
		Mesh1P->SetSkeletalMesh(Mesh1PObj.Object);
	}

	// 隐藏第三人称模型，只显示第一人称手臂
	GetMesh()->SetOwnerNoSee(true);


	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));

	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	GrenadeComponent = CreateDefaultSubobject<UGrenadeComponent>(TEXT("GrenadeComponent"));

	StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
	StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
	StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());

	ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonSpringArm"));
	ThirdPersonSpringArm->SetupAttachment(GetCapsuleComponent());
	ThirdPersonSpringArm->bUsePawnControlRotation = true;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonSpringArm);
	ThirdPersonCamera->SetAutoActivate(false);

	GetCharacterMovement()->CrouchedHalfHeight = 60.f;
	GetCharacterMovement()->MaxWalkSpeedCrouched = 300.f;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

}

bool As1mpleFpsCharacter::IsDead() const
{
	return HealthComponent && HealthComponent->bIsDead;
}

void As1mpleFpsCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 确保第一/第三人称 mesh 可见性在网络环境下正确（构造函数在 CDO 设置可能被 BP 覆盖）
	GetMesh()->SetOwnerNoSee(true);
	Mesh1P->SetOnlyOwnerSee(true);

	// 保存第三人称 Mesh 的默认相对变换，供 RespawnVisuals 还原
	DefaultMeshRelativeLocation = GetMesh()->GetRelativeLocation();
	DefaultMeshRelativeRotation = GetMesh()->GetRelativeRotation();

	// 如果 GrenadeComponent 裸指针为空（BP 编译/CDO 原因），从组件列表补上
	if (!GrenadeComponent)
	{
		GrenadeComponent = FindComponentByClass<UGrenadeComponent>();
	}


}

void As1mpleFpsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

//////////////////////////////////////////////////////////////////////////// Input

void As1mpleFpsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &As1mpleFpsCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &As1mpleFpsCharacter::Look);
		EnhancedInputComponent->BindAction(Interaction, ETriggerEvent::Started, this, &As1mpleFpsCharacter::Interact);
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &As1mpleFpsCharacter::Reload);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &As1mpleFpsCharacter::StartCrouch);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &As1mpleFpsCharacter::EndCrouch);
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &As1mpleFpsCharacter::PauseGame);

		EnhancedInputComponent->BindAction(WeaponSlot1Action, ETriggerEvent::Started, this,
			&As1mpleFpsCharacter::OnWeaponSlot1);
		EnhancedInputComponent->BindAction(WeaponSlot2Action, ETriggerEvent::Started, this,
			&As1mpleFpsCharacter::OnWeaponSlot2);
		EnhancedInputComponent->BindAction(WeaponSlot3Action, ETriggerEvent::Started, this,
			&As1mpleFpsCharacter::OnWeaponSlot3);
		EnhancedInputComponent->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &As1mpleFpsCharacter::NextWeapon);
		EnhancedInputComponent->BindAction(PrevWeaponAction, ETriggerEvent::Started, this,
			&As1mpleFpsCharacter::PreviousWeapon);
		EnhancedInputComponent->BindAction(ToggleViewAction, ETriggerEvent::Started, this,
			&As1mpleFpsCharacter::ToggleView);
		EnhancedInputComponent->BindAction(UseHealthAction, ETriggerEvent::Started, this,
			&As1mpleFpsCharacter::OnUseHealth);


		if (GrenadeThrowAction)
		{
			EnhancedInputComponent->BindAction(GrenadeThrowAction, ETriggerEvent::Started,
				this, &As1mpleFpsCharacter::OnThrowGrenade);
		}
		// 手雷相关输入 — 直接绑到 GrenadeComponent，Character 不做转发

		if (!GrenadeComponent)
			GrenadeComponent = FindComponentByClass<UGrenadeComponent>();

		if (GrenadeComponent)
		{


			if (GrenadeComponent->HighThrowAction)
			{
				EnhancedInputComponent->BindAction(GrenadeComponent->HighThrowAction, ETriggerEvent::Started,
					GrenadeComponent, &UGrenadeComponent::OnStartHighThrowMode);
				EnhancedInputComponent->BindAction(GrenadeComponent->HighThrowAction, ETriggerEvent::Completed,
					GrenadeComponent, &UGrenadeComponent::OnStopThrow);
			}
			if (GrenadeComponent->LowThrowAction)
			{
				EnhancedInputComponent->BindAction(GrenadeComponent->LowThrowAction, ETriggerEvent::Started,
					GrenadeComponent, &UGrenadeComponent::OnStartLowThrowMode);
				EnhancedInputComponent->BindAction(GrenadeComponent->LowThrowAction, ETriggerEvent::Completed,
					GrenadeComponent, &UGrenadeComponent::OnStopThrow);
			}
			if (GrenadeComponent->NextGrenadeAction)
			{
				EnhancedInputComponent->BindAction(GrenadeComponent->NextGrenadeAction, ETriggerEvent::Started,
					GrenadeComponent, &UGrenadeComponent::NextGrenade);
			}
			if (GrenadeComponent->PrevGrenadeAction)
			{
				EnhancedInputComponent->BindAction(GrenadeComponent->PrevGrenadeAction, ETriggerEvent::Started,
					GrenadeComponent, &UGrenadeComponent::PrevGrenade);
			}
		}
	}
	else
	{

	}
}

void As1mpleFpsCharacter::SwitchWeapon(int32 Index)
{
	// 手雷模式中：取消（已拉引信则阻止切武器）
	if (GrenadeComponent && GrenadeComponent->bIsEquipped)
	{
		GrenadeComponent->LeaveGrenadeMode();
		if (GrenadeComponent->bIsEquipped)  // LeaveGrenadeMode 失败 = 已拉引信
			return;
	}

	if (Index < 0 || Index >= WeaponInventory.Num()) return;
	UTP_WeaponComponent* NewWeapon = WeaponInventory[Index];
	if (!NewWeapon)return;
	if (NewWeapon == CurrentWeapon) return;
	if (CurrentWeapon) {
		CurrentWeapon->UnEquip();
	}
	NewWeapon->Equip();
	PreviousClientWeapon = CurrentWeapon;
	CurrentWeapon = NewWeapon;
	WeaponIndex = Index;

	if (IsLocallyControlled())
	{
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Controller))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->BindToWeapon(NewWeapon);
			}
		}
	}
}

void As1mpleFpsCharacter::NextWeapon()
{
	if (WeaponInventory.Num() <= 1) return;
	int32 NextIndex = (WeaponIndex + 1) % WeaponInventory.Num();
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		// 乐观本地切枪 + RPC，瞬时反馈
		SwitchWeapon(NextIndex);
		ServerNextWeapon();
		return;
	}
	SwitchWeapon(NextIndex);
}

void As1mpleFpsCharacter::PreviousWeapon()
{
	if (WeaponInventory.Num() <= 1) return;
	int32 PrevIndex = (WeaponIndex - 1 + WeaponInventory.Num()) % WeaponInventory.Num();
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		SwitchWeapon(PrevIndex);
		ServerPreviousWeapon();
		return;
	}
	SwitchWeapon(PrevIndex);
}

void As1mpleFpsCharacter::OnWeaponSlot1()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		SwitchWeapon(0);
		ServerSwitchWeapon(0);
		return;
	}
	SwitchWeapon(0);
}

void As1mpleFpsCharacter::OnWeaponSlot2()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		SwitchWeapon(1);
		ServerSwitchWeapon(1);
		return;
	}
	SwitchWeapon(1);
}

void As1mpleFpsCharacter::OnWeaponSlot3()
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		SwitchWeapon(2);
		ServerSwitchWeapon(2);
		return;
	}
	SwitchWeapon(2);
}

void As1mpleFpsCharacter::OnUseHealth()
{
	if (HealthComponent)
	{
		HealthComponent->OnUseHealth();
	}
}

void As1mpleFpsCharacter::Move(const FInputActionValue& Value)
{
	if (HealthComponent && HealthComponent->bIsDead) return;
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp) return;

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add movement
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
	float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFootstepTime > 0.4f && MovementVector.Size() > 0.5f)
	{
		UAISense_Hearing::ReportNoiseEvent(
			GetWorld(),
			GetActorLocation(),
			0.3f,
			this,
			0.0f,
			TEXT("Footstep")
		);
		LastFootstepTime = Now;
	}
}

void As1mpleFpsCharacter::Look(const FInputActionValue& Value)
{
	if (HealthComponent && HealthComponent->bIsDead) return;
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp) return;

	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void As1mpleFpsCharacter::PauseGame()
{
	As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetController());
	if (PC) {
		PC->TogglePause();
	}
	else {

	}
}

void As1mpleFpsCharacter::Interact()
{

	FVector CameraLocation = FirstPersonCameraComponent->GetComponentLocation();
	FVector CameraForward = FirstPersonCameraComponent->GetForwardVector();

	TArray<FOverlapResult> Overlap;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	bool bHasOverlap = GetWorld()->OverlapMultiByChannel(Overlap, CameraLocation + CameraForward * 150.f, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(200.f), Params);



	for (const auto& Hit : Overlap) {
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;


		UTP_PickUpComponent *PickUp = HitActor->FindComponentByClass<UTP_PickUpComponent>();
		if (!PickUp) {

			continue;
		}

		if (PickUp->bIsAlreadyPickedUp) {

			continue;
		}

		float Distance = FVector::Dist(CameraLocation, HitActor->GetActorLocation());
		if (Distance > PickUpDistance) {

			continue;
		}

		FVector ToTarget = (HitActor->GetActorLocation() - CameraLocation).GetSafeNormal();
		float Dot = FVector::DotProduct(CameraForward, ToTarget);
		if (Dot < 0.7f) {

			continue;
		}


		UTP_WeaponComponent* Weapon = HitActor->FindComponentByClass<UTP_WeaponComponent>();
		if (!Weapon) {

			continue;
		}
		if (WeaponInventory.Num() >= MaxWeaponSlots) {

			break;
		}

		if (HasAuthority() && IsLocallyControlled())
		{
			// Server-side locally-controlled player (Listen Server host).
			// Execute pickup through the shared implementation so that
			// bIsAlreadyPickedUp, ClientSyncWeaponAmmo, FlushNetDormancy,
			// and ForceNetUpdate all fire correctly.

			ServerPickUpWeapon_Implementation(HitActor);
		}
		else
		{
			// Pure client: RPC to server + optimistic local pickup for responsiveness.
			// If server rejects, ClientUndoPickUp handles rollback.

			ServerPickUpWeapon(HitActor);

			// Optimistic local pickup (server will validate)
			if (!Weapon->AttachWeapon(this)) {

				break;
			}
			WeaponInventory.Add(Weapon);
			SwitchWeapon(WeaponInventory.Num() - 1);
			PickUp->OnPickUp.Broadcast(this);

			PickUp->bIsAlreadyPickedUp = true;
			HitActor->SetActorEnableCollision(false);

		}
		break;
	}
}

void As1mpleFpsCharacter::Reload()
{
	if (CurrentWeapon) {
		CurrentWeapon->Reload();
	}
	else
	{
	}
}

void As1mpleFpsCharacter::PlayHitMarker(bool bIsEnemy)
{
	ShowHitMarket(bIsEnemy);
}

void As1mpleFpsCharacter::ToggleView()
{
	// 防止快速连点导致相机状态混乱（0.25 秒冷却）
	float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Now - LastViewToggleTime < 0.25f) return;
	LastViewToggleTime = Now;

	// 切视角前强制退出 ADS，防止 FOV/SpringArm 状态混乱
	if (CurrentWeapon && CurrentWeapon->bIsAiming)
	{
		CurrentWeapon->EndAiming();
	}

	bIsThirdPerson = !bIsThirdPerson;
	if (bIsThirdPerson)
	{
		ThirdPersonCamera->SetActive(true);
		FirstPersonCameraComponent->SetActive(false);
		Mesh1P->SetVisibility(false);
		GetMesh()->SetOwnerNoSee(false);
		ThirdPersonSpringArm->bUsePawnControlRotation = true;
		ReattachWeaponsForView(true);
	}
	else
	{
		ThirdPersonCamera->SetActive(false);
		FirstPersonCameraComponent->SetActive(true);
		Mesh1P->SetVisibility(true);
		GetMesh()->SetOwnerNoSee(true);
		ThirdPersonSpringArm->bUsePawnControlRotation = false;
		ReattachWeaponsForView(false);
	}
}

void As1mpleFpsCharacter::ReattachWeaponsForView(bool bToThirdPerson)
{
	if (bToThirdPerson)
	{
		// 第三人称：隐藏所有武器（没有配套动画，全部不可见）
		for (UTP_WeaponComponent* Weapon : WeaponInventory)
		{
			if (!Weapon) continue;
			Weapon->SetVisibility(false);
			Weapon->SetHiddenInGame(true, true);
		}
	}
	else
	{
		// 第一人称：武器挂回 Mesh1P，仅持有者可见
		for (UTP_WeaponComponent* Weapon : WeaponInventory)
		{
			if (!Weapon) continue;
			bool bIsActive = (Weapon == CurrentWeapon);
			Weapon->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("GripPoint")));
			Weapon->ApplyWeaponMeshTransform();
			Weapon->SetVisibility(bIsActive);
			Weapon->SetHiddenInGame(false, true);
			Weapon->SetOnlyOwnerSee(true);
		}
	}
}

// --- Network Replication ---

void As1mpleFpsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(As1mpleFpsCharacter, CurrentWeapon);
	DOREPLIFETIME(As1mpleFpsCharacter, WeaponInventory);
	DOREPLIFETIME(As1mpleFpsCharacter, WeaponIndex);
}

void As1mpleFpsCharacter::OnRep_CurrentWeapon()
{

	if (IsLocallyControlled())
	{
		// 本地玩家：武器可能在客户端没 Equip（纯客户端只发 RPC），OnRep 时补 Equip
		if (CurrentWeapon != PreviousClientWeapon)
		{
			if (PreviousClientWeapon)
				PreviousClientWeapon->UnEquip();
			if (CurrentWeapon)
			{
				CurrentWeapon->SetOwningCharacter(this);
				CurrentWeapon->Equip();
			}
			PreviousClientWeapon = CurrentWeapon;
		}
		if (CurrentWeapon)
		{
			if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Controller))
			{
				if (PC->HUDWidget)
				{
					PC->HUDWidget->BindToWeapon(CurrentWeapon);
				}
			}
		}
		return;
	}

	if (CurrentWeapon != PreviousClientWeapon)
	{
		if (PreviousClientWeapon)
			PreviousClientWeapon->UnEquip();
		if (CurrentWeapon)
		{

			CurrentWeapon->SetOwningCharacter(this);
			CurrentWeapon->Equip();
		}
		PreviousClientWeapon = CurrentWeapon;
	}
}

void As1mpleFpsCharacter::OnRep_WeaponInventory()
{

	if (IsLocallyControlled())
	{
		// Handle pending purchase (ClientPurchaseComplete arrived before replication)
		if (PendingPurchaseIndex >= 0)
		{
			if (WeaponInventory.IsValidIndex(PendingPurchaseIndex) && WeaponInventory[PendingPurchaseIndex])
			{

				UTP_WeaponComponent* Weapon = WeaponInventory[PendingPurchaseIndex];
				Weapon->SetOwningCharacter(this);
				CurrentWeapon = nullptr;
				SwitchWeapon(PendingPurchaseIndex);
				PendingPurchaseIndex = -1;
				GetWorldTimerManager().ClearTimer(PurchaseRetryHandle);
			}
			else
			{
				// 动态创建的组件可能尚未复制到达，延迟重试

				if (PurchaseRetryCount < 10)
				{
					PurchaseRetryCount++;
					GetWorldTimerManager().SetTimer(PurchaseRetryHandle, [this]() {
						OnRep_WeaponInventory();
					}, 0.1f, false);
				}
				else
				{

					PendingPurchaseIndex = -1;
					PurchaseRetryCount = 0;
				}
			}
		}
		else
		{
			PurchaseRetryCount = 0;
		}
		return;
	}

	// 远程角色：所有武器隐藏（无配套动画，不应显示）
	for (UTP_WeaponComponent* Weapon : WeaponInventory)
	{
		if (!Weapon) continue;
		if (Weapon->GetAttachParent() != Mesh1P)
		{
			Weapon->SetOwningCharacter(this);
			Weapon->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("GripPoint")));
			Weapon->ApplyWeaponMeshTransform();
			Weapon->bIsEquipped = false;
		}
		Weapon->SetVisibility(false);
		Weapon->SetHiddenInGame(true, true);
	}

	// Ensure CurrentWeapon reflects the server's WeaponIndex
	if (WeaponInventory.IsValidIndex(WeaponIndex) && WeaponInventory[WeaponIndex] != CurrentWeapon)
	{
		if (CurrentWeapon)
			CurrentWeapon->UnEquip();
		CurrentWeapon = WeaponInventory[WeaponIndex];
		if (CurrentWeapon)
		{
			CurrentWeapon->SetOwningCharacter(this);
			CurrentWeapon->Equip();
		}
		PreviousClientWeapon = CurrentWeapon;
	}
}

void As1mpleFpsCharacter::SetActiveWeapon(UTP_WeaponComponent* NewWeapon)
{
	if (NewWeapon != CurrentWeapon)
	{
		if (CurrentWeapon)
			CurrentWeapon->UnEquip();
		CurrentWeapon = NewWeapon;
		if (CurrentWeapon)
			CurrentWeapon->Equip();
	}
}

void As1mpleFpsCharacter::ServerPickUpWeapon_Implementation(AActor* HitActor)
{

	if (!HitActor) return;

	// Guard against double-pickup (race condition between two players)
	UTP_PickUpComponent* PickUp = HitActor->FindComponentByClass<UTP_PickUpComponent>();
	if (PickUp && PickUp->bIsAlreadyPickedUp)
	{

		ClientUndoPickUp(HitActor);
		return;
	}

	UTP_WeaponComponent* PickupWeapon = HitActor->FindComponentByClass<UTP_WeaponComponent>();
	if (!PickupWeapon)
	{
		ClientUndoPickUp(HitActor);
		return;
	}
	if (WeaponInventory.Num() >= MaxWeaponSlots)
	{
		ClientUndoPickUp(HitActor);
		return;
	}

	// Reuse the pickup weapon component directly (no NewObject).
	// Dynamically created components cannot be properly replicated in UE5
	// without full subobject registration, which requires engine-level setup.
	// The pickup weapon already exists on both server and client, and
	// AttachToComponent replicates the scene attachment.  The key sync
	// guarantees come from bIsAlreadyPickedUp (now replicated) and the
	// weapon switching Server RPCs.
	if (!PickupWeapon->AttachWeapon(this))
	{
		ClientUndoPickUp(HitActor);
		return;
	}

	WeaponInventory.Add(PickupWeapon);
	SwitchWeapon(WeaponInventory.Num() - 1);

	if (PickUp)
	{
		PickUp->OnPickUp.Broadcast(this);
		PickUp->bIsAlreadyPickedUp = true;
	}
	HitActor->SetActorEnableCollision(false);
	HitActor->FlushNetDormancy();
	HitActor->ForceNetUpdate();

	MulticastOnPickUp(HitActor);
	ClientSyncWeaponAmmo(WeaponIndex, PickupWeapon->CurrentAmmo, PickupWeapon->SpareAmmo);


}

void As1mpleFpsCharacter::ClientUndoPickUp_Implementation(AActor* HitActor)
{

	if (!HitActor) return;

	UTP_WeaponComponent* Weapon = HitActor->FindComponentByClass<UTP_WeaponComponent>();
	if (!Weapon) return;

	// Remove from local inventory if present (server rejected our optimistic pickup)
	int32 Idx = WeaponInventory.Find(Weapon);
	if (Idx != INDEX_NONE)
	{
		if (CurrentWeapon == Weapon)
		{
			CurrentWeapon->UnEquip();
			CurrentWeapon = nullptr;
		}
		WeaponInventory.RemoveAt(Idx);
	}
	// If we removed the active weapon, switch to a remaining one
	if (CurrentWeapon == nullptr && WeaponInventory.Num() > 0)
	{
		int32 FallbackIdx = FMath::Min(Idx, WeaponInventory.Num() - 1);
		SwitchWeapon(FallbackIdx);
	}
	// Note: do NOT re-enable collision or reset bIsAlreadyPickedUp here.
	// The server authoritatively controls the pickup actor's state.
	// If the server rejected because another player already took the weapon,
	// we must not make it appear available again.
}

void As1mpleFpsCharacter::MulticastOnPickUp_Implementation(AActor* HitActor)
{


	if (IsLocallyControlled())
	{

		return;
	}
	if (HasAuthority())
	{

		return;
	}
	if (!HitActor) return;

	UTP_PickUpComponent* PickUp = HitActor->FindComponentByClass<UTP_PickUpComponent>();


	if (PickUp)
	{
		PickUp->bIsAlreadyPickedUp = true;
		PickUp->OnPickUp.Broadcast(this);
	}
	HitActor->SetActorEnableCollision(false);

	// Manually attach the weapon component to this character's mesh on
	// the client.  Scene-attachment replication from the pickup actor can
	// be unreliable for Blueprint-placed actors; doing the attach directly
	// in the Multicast RPC guarantees the weapon mesh moves to the
	// character's hand on every client, regardless of replication order.
	UTP_WeaponComponent* Weapon = HitActor->FindComponentByClass<UTP_WeaponComponent>();
	if (Weapon)
	{
		USceneComponent* OldParent = Weapon->GetAttachParent();


		Weapon->AttachToComponent(GetMesh1P(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("GripPoint")));
		Weapon->ApplyWeaponMeshTransform();
		Weapon->SetOnlyOwnerSee(true);
	}


}

void As1mpleFpsCharacter::ServerSwitchWeapon_Implementation(int32 Index)
{
	SwitchWeapon(Index);
	if (CurrentWeapon)
	{
		ClientSyncWeaponAmmo(WeaponIndex, CurrentWeapon->CurrentAmmo, CurrentWeapon->SpareAmmo);
	}
}

void As1mpleFpsCharacter::ServerPreviousWeapon_Implementation()
{
	PreviousWeapon();
	if (CurrentWeapon)
	{
		ClientSyncWeaponAmmo(WeaponIndex, CurrentWeapon->CurrentAmmo, CurrentWeapon->SpareAmmo);
	}
}

void As1mpleFpsCharacter::ServerNextWeapon_Implementation()
{
	NextWeapon();
	if (CurrentWeapon)
	{
		ClientSyncWeaponAmmo(WeaponIndex, CurrentWeapon->CurrentAmmo, CurrentWeapon->SpareAmmo);
	}
}

void As1mpleFpsCharacter::ClientSyncWeaponAmmo_Implementation(int32 InWeaponIndex, int32 InCurrentAmmo, int32 InSpareAmmo)
{
	if (!WeaponInventory.IsValidIndex(InWeaponIndex)) return;
	UTP_WeaponComponent* Weapon = WeaponInventory[InWeaponIndex];
	if (!Weapon) return;
	Weapon->CurrentAmmo = InCurrentAmmo;
	Weapon->SpareAmmo = InSpareAmmo;
	Weapon->OnAmmoChanged.Broadcast(InCurrentAmmo, InSpareAmmo);
}

void As1mpleFpsCharacter::ServerFireWeapon_Implementation(int32 InWeaponIndex, FVector SpawnLocation, FRotator SpawnRotation)
{
	// 优先用客户端指定的武器（InWeaponIndex），因为 CurrentWeapon 可能因切枪延迟而不一致
	// 只有槽位无效时才回退到 CurrentWeapon
	UTP_WeaponComponent* Weapon = nullptr;
	if (WeaponInventory.IsValidIndex(InWeaponIndex))
	{
		Weapon = WeaponInventory[InWeaponIndex];
	}
	if (!Weapon)
	{
		Weapon = CurrentWeapon;

	}
	if (!Weapon)
	{

		return;
	}
	Weapon->ServerFire(SpawnLocation, SpawnRotation);
}

void As1mpleFpsCharacter::ServerReloadWeapon_Implementation(int32 InWeaponIndex)
{
	UTP_WeaponComponent* Weapon = nullptr;
	if (WeaponInventory.IsValidIndex(InWeaponIndex))
	{
		Weapon = WeaponInventory[InWeaponIndex];
	}
	if (!Weapon)
	{
		Weapon = CurrentWeapon;
	}
	if (!Weapon || !Weapon->WeaponData) return;

	// 直接执行换弹，不走 timer。
	// 客户端用自己的 timer 控制动画/HUD，服务器立即恢复弹药。
	// 这样客户端的开火 RPC 到达时服务器已经有弹药，不会丢弹。
	if (Weapon->ReplicatedSpareAmmo <= 0) return;
	if (Weapon->ReplicatedCurrentAmmo >= Weapon->WeaponData->MaxProjectile) return;

	int32 Needed = Weapon->WeaponData->MaxProjectile - Weapon->ReplicatedCurrentAmmo;
	int32 ToReload = FMath::Min(Needed, Weapon->ReplicatedSpareAmmo);

	// 清除可能正在进行的服务器端换弹 timer
	Weapon->CancelReload();

	Weapon->ReplicatedCurrentAmmo += ToReload;
	Weapon->ReplicatedSpareAmmo -= ToReload;
	Weapon->CurrentAmmo = Weapon->ReplicatedCurrentAmmo;
	Weapon->SpareAmmo = Weapon->ReplicatedSpareAmmo;
	Weapon->OnAmmoChanged.Broadcast(Weapon->CurrentAmmo, Weapon->SpareAmmo);


}

// ===== 购买系统 =====

int32 As1mpleFpsCharacter::GrantWeapon(TSubclassOf<UTP_WeaponComponent> WeaponClass, UWeaponData* WeaponDataPtr)
{
	if (!WeaponClass) return -1;
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return -1;

	int32 TargetSlot = INDEX_NONE;
	for (int32 i = 0; i < MaxWeaponSlots; i++)
	{
		if (i >= WeaponInventory.Num() || WeaponInventory[i] == nullptr)
		{
			TargetSlot = i;
			break;
		}
	}
	if (TargetSlot == INDEX_NONE)
		TargetSlot = WeaponIndex;

	RemoveWeaponSlot(TargetSlot);

	UTP_WeaponComponent* NewWeapon = NewObject<UTP_WeaponComponent>(this, WeaponClass);
	NewWeapon->SetIsReplicated(true);
	if (WeaponDataPtr)
		NewWeapon->WeaponData = WeaponDataPtr;
	NewWeapon->RegisterComponent();
	NewWeapon->AttachWeapon(this);

	while (WeaponInventory.Num() <= TargetSlot)
		WeaponInventory.Add(nullptr);

	WeaponInventory[TargetSlot] = NewWeapon;
	SwitchWeapon(TargetSlot);
	ClientSyncWeaponAmmo(WeaponIndex, NewWeapon->CurrentAmmo, NewWeapon->SpareAmmo);
	return TargetSlot;
}

void As1mpleFpsCharacter::RemoveWeaponSlot(int32 RemoveIndex)
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;
	if (RemoveIndex < 0 || RemoveIndex >= WeaponInventory.Num()) return;

	UTP_WeaponComponent* OldWeapon = WeaponInventory[RemoveIndex];
	if (!OldWeapon) return;

	if (CurrentWeapon == OldWeapon)
	{
		CurrentWeapon->UnEquip();
		CurrentWeapon = nullptr;
	}
	OldWeapon->DestroyComponent();
	WeaponInventory[RemoveIndex] = nullptr;

	// 如果删除的是当前武器槽，切换到最近的可用武器
	if (RemoveIndex == WeaponIndex)
	{
		int32 NewIndex = -1;
		for (int32 i = 0; i < WeaponInventory.Num(); i++)
		{
			if (WeaponInventory[i] != nullptr)
			{
				NewIndex = i;
				break;
			}
		}
		if (NewIndex >= 0)
			SwitchWeapon(NewIndex);
		else
			WeaponIndex = 0; // 无武器可用，重置索引
	}
}

void As1mpleFpsCharacter::GrantArmor(UArmorData* ArmorDataPtr)
{
	if (!ArmorDataPtr) return;
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;
	DamageComponent->EquippedArmors.AddUnique(ArmorDataPtr);
}

void As1mpleFpsCharacter::GrantHealthItem(UHealthData* HealthDataPtr)
{
	if (HealthComponent)
	{
		HealthComponent->GrantHealthItem(HealthDataPtr);
	}
}

void As1mpleFpsCharacter::ClientApplyFlash_Implementation(float Intensity, float Duration)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	UFlashWidget* FlashWidget = CreateWidget<UFlashWidget>(PC, UFlashWidget::StaticClass());
	if (FlashWidget)
	{
		FlashWidget->AddToViewport();
		FlashWidget->StartFlashing(Intensity, Duration);
	}
}

void As1mpleFpsCharacter::ClientDamageFeedback_Implementation(float Intensity, float Duration)
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;
	UFlashWidget* FlashWidget = CreateWidget<UFlashWidget>(PC, UFlashWidget::StaticClass());
	if (FlashWidget)
	{
		FlashWidget->AddToViewport();
		FlashWidget->StartFlashing(Intensity, Duration, FLinearColor(0.8f, 0.0f, 0.0f));  // 红色血反馈
	}
}
