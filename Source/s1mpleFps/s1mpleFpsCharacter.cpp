// Copyright Epic Games, Inc. All Rights Reserved.

#include "s1mpleFpsCharacter.h"
#include "DamageComponent.h"
#include "HealthComponent.h"
#include "WeaponInventoryComponent.h"
#include "TP_WeaponComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "UObject/ConstructorHelpers.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "HealthData.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "s1mpleFpsPlayerController.h"
#include "HUDWidget.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "s1mpleFpsGameState.h"
#include "GameFramework/SpringArmComponent.h"
#include "FlashWidget.h"
#include "GrenadeComponent.h"
#include "SettingsSubsystem.h"
#include "Engine/GameInstance.h"

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
	NetUpdateFrequency = 30.0f; // 网络带宽优化：角色默认 100 → 30

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

	WeaponInventoryComponent = CreateDefaultSubobject<UWeaponInventoryComponent>(TEXT("WeaponInventoryComponent"));

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

	// 热身登场动画：开局热身阶段，第三人称模型播一次登场 montage（服务器广播到所有端）
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp && ThirdPersonIntroMontage && HasAuthority())
	{
		MulticastPlayThirdPersonMontage(ThirdPersonIntroMontage);
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
		if (TauntAction)
		{
			EnhancedInputComponent->BindAction(TauntAction, ETriggerEvent::Started, this,
				&As1mpleFpsCharacter::OnTaunt);
		}


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

void As1mpleFpsCharacter::NextWeapon()
{
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->NextWeapon();
}

void As1mpleFpsCharacter::PreviousWeapon()
{
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->PreviousWeapon();
}

void As1mpleFpsCharacter::OnWeaponSlot1()
{
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(0);
}

void As1mpleFpsCharacter::OnWeaponSlot2()
{
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(1);
}

void As1mpleFpsCharacter::OnWeaponSlot3()
{
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(2);
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

	// 鼠标灵敏度（从设置存档读取）
	float Sensitivity = 1.0f;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (USettingsSubsystem* Settings = GI->GetSubsystem<USettingsSubsystem>())
		{
			Sensitivity = Settings->GetMouseSensitivity();
		}
	}

	if (Controller != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X * Sensitivity);
		AddControllerPitchInput(LookAxisVector.Y * Sensitivity);
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
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->Interact();
}

void As1mpleFpsCharacter::Reload()
{
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->Reload();
}

void As1mpleFpsCharacter::PlayHitMarker_Implementation(bool bIsEnemy)
{
	// 命中标记统一显示在 HUD（UHUDWidget::PlayHitMarker 负责淡入淡出 + 敌人红/墙白配色）
	if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetController()))
	{
		if (PC->HUDWidget)
		{
			PC->HUDWidget->PlayHitMarker(bIsEnemy);
		}
	}
}

void As1mpleFpsCharacter::ToggleView()
{
	// 防止快速连点导致相机状态混乱（0.25 秒冷却）
	float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Now - LastViewToggleTime < 0.25f) return;
	LastViewToggleTime = Now;

	// 切视角前强制退出 ADS，防止 FOV/SpringArm 状态混乱
	UTP_WeaponComponent* ActiveWeapon = WeaponInventoryComponent ? WeaponInventoryComponent->CurrentWeapon : nullptr;
	if (ActiveWeapon && ActiveWeapon->bIsAiming)
	{
		ActiveWeapon->EndAiming();
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
	if (!WeaponInventoryComponent) return;

	if (bToThirdPerson)
	{
		// 第三人称：隐藏所有武器（没有配套动画，全部不可见）
		for (UTP_WeaponComponent* Weapon : WeaponInventoryComponent->WeaponInventory)
		{
			if (!Weapon) continue;
			Weapon->SetVisibility(false);
			Weapon->SetHiddenInGame(true, true);
		}
	}
	else
	{
		// 第一人称：武器挂回 Mesh1P，仅持有者可见
		for (UTP_WeaponComponent* Weapon : WeaponInventoryComponent->WeaponInventory)
		{
			if (!Weapon) continue;
			bool bIsActive = (Weapon == WeaponInventoryComponent->CurrentWeapon);
			Weapon->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("GripPoint")));
			Weapon->ApplyWeaponMeshTransform();
			Weapon->SetVisibility(bIsActive);
			Weapon->SetHiddenInGame(false, true);
			Weapon->SetOnlyOwnerSee(true);
		}
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

void As1mpleFpsCharacter::PlayThirdPersonMontage(UAnimMontage* Montage)
{
	if (!IsValid(Montage)) return;
	UAnimInstance* AnimInst = GetMesh()->GetAnimInstance();
	if (!AnimInst)
	{
		UE_LOG(LogTemplateCharacter, Warning, TEXT("[ThirdPersonAnim] %s GetMesh()->GetAnimInstance() 为空，montage 无法播放"), *GetName());
		return;
	}
	UE_LOG(LogTemplateCharacter, Log, TEXT("[ThirdPersonAnim] %s 播放 montage %s"), *GetName(), *Montage->GetName());
	AnimInst->Montage_Play(Montage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
}

void As1mpleFpsCharacter::MulticastThirdPersonFire_Implementation(UAnimMontage* FireMontage)
{
	// 第一人称时本地玩家看的是手臂，不播自己身体；第三人称时本地玩家也要看到
	if (IsLocallyControlled() && !bIsThirdPerson) return;
	PlayThirdPersonMontage(FireMontage);
}

void As1mpleFpsCharacter::MulticastMuzzleFlash_Implementation(FVector SpawnLocation, FRotator SpawnRotation, UParticleSystem* MuzzleFlash, USoundBase* FireSound)
{
	// 本地玩家在 StartSingleFire 里已经直接 spawn 了枪口火焰，这里只给其他端看
	if (IsLocallyControlled()) return;

	// 优先用第三人称骨骼的 muzzle 插槽对齐枪口（需在第三人称骨骼上加 muzzle 插槽）；没有则退回传入位置
	FVector FlashLocation = SpawnLocation;
	if (GetMesh() && GetMesh()->DoesSocketExist(FName("muzzle")))
	{
		FlashLocation = GetMesh()->GetSocketLocation(FName("muzzle"));
	}

	if (MuzzleFlash && GetWorld())
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, FlashLocation, SpawnRotation);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, FlashLocation);
	}
}

void As1mpleFpsCharacter::MulticastPlayThirdPersonMontage_Implementation(UAnimMontage* Montage)
{
	PlayThirdPersonMontage(Montage);
}

void As1mpleFpsCharacter::MulticastPlayDeathMontage_Implementation(UAnimMontage* DeathMontage)
{
	if (IsValid(DeathMontage))
	{
		PlayThirdPersonMontage(DeathMontage);
		const float Len = FMath::Max(DeathMontage->GetPlayLength(), 0.1f);
		GetWorld()->GetTimerManager().SetTimer(DeathHideTimerHandle, [this]()
		{
			GetMesh()->SetVisibility(false);
			GetMesh()->SetHiddenInGame(true, true);
			GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}, Len, false);
	}
	else
	{
		// 没配死亡动画：立即隐藏第三人称 mesh
		GetMesh()->SetVisibility(false);
		GetMesh()->SetHiddenInGame(true, true);
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

void As1mpleFpsCharacter::OnTaunt()
{
	if (IsDead()) return;
	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (Now - LastTauntTime < 2.0f) return; // 防连点
	LastTauntTime = Now;

	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		ServerTaunt();
		return;
	}
	// 服务器（含 Listen Server 主机）：轮换选择一个嘲讽 montage 广播
	if (ThirdPersonTauntMontages.Num() == 0) return;
	const int32 Idx = (LastTauntIndex + 1) % ThirdPersonTauntMontages.Num();
	LastTauntIndex = Idx;
	MulticastPlayThirdPersonMontage(ThirdPersonTauntMontages[Idx]);
}

void As1mpleFpsCharacter::ServerTaunt_Implementation()
{
	if (IsDead()) return;
	if (ThirdPersonTauntMontages.Num() == 0) return;
	const int32 Idx = (LastTauntIndex + 1) % ThirdPersonTauntMontages.Num();
	LastTauntIndex = Idx;
	MulticastPlayThirdPersonMontage(ThirdPersonTauntMontages[Idx]);
}
