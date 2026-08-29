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
#include "HeroData.h"
#include "Door.h"
#include "Net/UnrealNetwork.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

//////////////////////////////////////////////////////////////////////////
// As1mpleFpsCharacter

void As1mpleFpsCharacter::OnThrowGrenade(const FInputActionValue& Value)
{
	if (bIsClimbing || bClimbRequestPending) return;
	if (GrenadeComponent)
		GrenadeComponent->ToggleGrenadeMode();
}


As1mpleFpsCharacter::As1mpleFpsCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SetNetUpdateFrequency(30.0f); // 网络带宽优化：角色默认 100 → 30

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->SetGenerateOverlapEvents(true);

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

	GetCharacterMovement()->SetCrouchedHalfHeight(60.f);
	GetCharacterMovement()->MaxWalkSpeedCrouched = 300.f;
	GetCharacterMovement()->bCanWalkOffLedgesWhenCrouching = true;
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;

}

void As1mpleFpsCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(As1mpleFpsCharacter, SelectedHeroMesh);
	DOREPLIFETIME(As1mpleFpsCharacter, SelectedHeroAnimClass);
	DOREPLIFETIME(As1mpleFpsCharacter, SelectedHeroPhysicsAsset);
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

	// 应用第三人称相机参数（BP 里调的值，写到弹簧臂/相机）
	if (ThirdPersonSpringArm)
	{
		ThirdPersonSpringArm->TargetArmLength = ThirdPersonArmLength;
		ThirdPersonSpringArm->SocketOffset = ThirdPersonSocketOffset;
		ThirdPersonSpringArm->TargetOffset = ThirdPersonTargetOffset;
	}
	if (ThirdPersonCamera)
	{
		ThirdPersonCamera->FieldOfView = ThirdPersonFOV;
	}

	// 应用选中英雄/皮肤的第三人称模型：服务器解析并复制，客户端靠 OnRep_HeroVisual 套用
	if (HasAuthority())
	{
		ApplyHeroVisual();
	}
	else
	{
		ApplyHeroVisualNow();
	}

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
	
	if (bIsClimbing)
	{
		if (IsDead())
		{
			StopClimbing(false);
		}
		else
		{
			ClimbElapsedTime += DeltaTime;
			const float MoveDuration = FMath::Max(ClimbDuration, 0.01f);
			float Alpha = FMath::Clamp(ClimbElapsedTime / MoveDuration, 0.0f, 1.0f);

			// 两段式：先竖直升到墙沿上方（贴着墙面、不穿墙），再水平移上墙顶落点
			FVector NewLocation;
			if (Alpha < 0.5f)
			{
				float t = Alpha / 0.5f;
				NewLocation = FMath::Lerp(StartClimbLocation, ClimbOverLocation, t);
			}
			else
			{
				float t = (Alpha - 0.5f) / 0.5f;
				NewLocation = FMath::Lerp(ClimbOverLocation, TargetClimbLocation, t);
			}
			// 不用 Sweep：攀爬是两段式、路径已避开墙面，Sweep 会在半路被墙挡住导致卡在空中
			SetActorLocation(NewLocation, false);

			if (Alpha >= 1.0f)
			{
				SetActorLocation(TargetClimbLocation, false); // 精确落到最终落点
				StopClimbing(true);
			}
		}
		return; // 攀爬时跳过其余逻辑
	}
	// 英雄模型要等 PlayerState 就位才解析。
	// 服务端按「已套用索引 != 当前索引」判断是否要重新套用（无缝跳图时索引可能晚于角色 BeginPlay 才恢复）。
	if (HasAuthority())
	{
		As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
		if (PS && AppliedHeroIndex != PS->SelectedHeroIndex)
		{
			ApplyHeroVisual();
		}
	}
}

void As1mpleFpsCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 清掉「死亡后隐藏 mesh」的定时器，防止角色被销毁/切图后定时器回调访问已释放的 this（Timer UAF 崩溃）
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DeathHideTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

//////////////////////////////////////////////////////////////////////////// Input

void As1mpleFpsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &As1mpleFpsCharacter::OnJumpPressed);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &As1mpleFpsCharacter::StopJumping);

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
		if (DoorAction) {
			EnhancedInputComponent->BindAction(DoorAction, ETriggerEvent::Started, this,
				&As1mpleFpsCharacter::HandleDoor);
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
	if (bIsClimbing || bClimbRequestPending) return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->NextWeapon();
}

void As1mpleFpsCharacter::PreviousWeapon()
{
	if (bIsClimbing || bClimbRequestPending) return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->PreviousWeapon();
}

void As1mpleFpsCharacter::OnWeaponSlot1()
{
	if (bIsClimbing || bClimbRequestPending) return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(0);
}

void As1mpleFpsCharacter::OnWeaponSlot2()
{
	if (bIsClimbing || bClimbRequestPending) return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(1);
}

void As1mpleFpsCharacter::OnWeaponSlot3()
{
	if (bIsClimbing || bClimbRequestPending) return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(2);
}

void As1mpleFpsCharacter::OnUseHealth()
{
	if (bIsClimbing || bClimbRequestPending) return;
	if (HealthComponent)
	{
		HealthComponent->OnUseHealth();
	}
}

void As1mpleFpsCharacter::Move(const FInputActionValue& Value)
{
	if (bIsClimbing || bClimbRequestPending) return;
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
	if (bIsClimbing || bClimbRequestPending) return;
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
	if (bIsClimbing || bClimbRequestPending) return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->Interact();
}

void As1mpleFpsCharacter::Reload()
{
	if (bIsClimbing || bClimbRequestPending) return;
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

	SetViewMode(!bIsThirdPerson);
}

void As1mpleFpsCharacter::SetViewMode(bool bToThirdPerson)
{
	bIsThirdPerson = bToThirdPerson;
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

	// 攀爬中切换视角：停掉旧视角动画，按新视角重播对应动画
	if (bIsClimbing)
	{
		UAnimMontage* FPPMontage = GetFPPClimbMontage();
		UAnimMontage* TPPMontage = GetTPPClimbMontage();
		if (Mesh1P && FPPMontage && Mesh1P->GetAnimInstance())
			Mesh1P->GetAnimInstance()->Montage_Stop(0.0f, FPPMontage);
		if (GetMesh() && TPPMontage && GetMesh()->GetAnimInstance())
			GetMesh()->GetAnimInstance()->Montage_Stop(0.0f, TPPMontage);

		if (IsLocallyControlled())
		{
			if (bIsThirdPerson && TPPMontage)
				PlayThirdPersonMontage(TPPMontage);
			else if (FPPMontage && Mesh1P && Mesh1P->GetAnimInstance())
				Mesh1P->GetAnimInstance()->Montage_Play(FPPMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
		}
		else if (TPPMontage)
		{
			PlayThirdPersonMontage(TPPMontage);
		}
	}
}

void As1mpleFpsCharacter::ForceToFirstPerson()
{
	if (bIsThirdPerson)
	{
		SetViewMode(false);
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

void As1mpleFpsCharacter::ApplyHeroVisual()
{
	// 只服务器解析；客户端靠 OnRep_HeroVisual 套用复制过来的模型
	if (!HasAuthority()) return;

	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS) return;

	As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetController());
	if (!PC || PC->HeroRoster.Num() == 0) return;

	UHeroData* Hero = PC->GetHeroByIndex(PS->SelectedHeroIndex);
	if (!Hero || !Hero->Mesh) return;

	// 写入复制字段 → 客户端 OnRep_HeroVisual 自动套用；本地也立即套用
	SelectedHeroMesh = Hero->Mesh;
	SelectedHeroAnimClass = Hero->AnimInstanceClass;
	SelectedHeroPhysicsAsset = Hero->PhysicsAsset;
	ApplyHeroVisualNow();
	AppliedHeroIndex = PS->SelectedHeroIndex; // 记录已套用的索引，索引变化时 Tick 会重新套用
	UE_LOG(LogTemplateCharacter, Log, TEXT("[Hero] 服务端套用: idx=%d mesh=%s (%s)"),
		PS->SelectedHeroIndex, *Hero->Mesh->GetName(), *GetName());
}

void As1mpleFpsCharacter::ApplyHeroVisualNow()
{
	if (!SelectedHeroMesh) return;
	GetMesh()->SetSkeletalMesh(SelectedHeroMesh);
	if (SelectedHeroPhysicsAsset)
	{
		GetMesh()->SetPhysicsAsset(SelectedHeroPhysicsAsset);
	}
	if (SelectedHeroAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(SelectedHeroAnimClass);
	}
}

void As1mpleFpsCharacter::OnRep_HeroVisual()
{
	UE_LOG(LogTemplateCharacter, Log, TEXT("[Hero] OnRep_HeroVisual: mesh=%s net=%d (%s)"),
		SelectedHeroMesh ? *SelectedHeroMesh->GetName() : TEXT("NULL"), (int32)GetNetMode(), *GetName());
	ApplyHeroVisualNow();
}

void As1mpleFpsCharacter::HandleDoor()
{
	if (bIsClimbing || bClimbRequestPending) return;
	As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(GetController());
	if (!PC)return;

	TArray<AActor*> OverlapActors;
	GetOverlappingActors(OverlapActors, ADoor::StaticClass());
	for (AActor *Actor:OverlapActors) {
		ADoor* Door = Cast<ADoor>(Actor);
		if (Door) {
			Door->Door_Interact(PC);
			break;
		}
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

void As1mpleFpsCharacter::ClientPlayKillSound_Implementation(USoundBase* Sound)
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
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
	if (bIsClimbing || bClimbRequestPending) return;
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

void As1mpleFpsCharacter::OnJumpPressed()
{
	if (IsDead()) return;
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp) return;

	// 已在攀爬中：按跳跃取消
	if (bIsClimbing)
	{
		if (HasAuthority())
			MulticastStopClimbing(false);
		else
			ServerCancelClimbing();
		return;
	}

	// 已有请求在处理，防止重复触发
	if (bClimbRequestPending) return;

	// 本地检测（提前锁定输入，减少无效 RPC）
	FVector Target, OverTarget;
	if (CanClimbing(Target, OverTarget))
	{
		bClimbRequestPending = true;   // 锁定输入，直到服务器确认/拒绝

		if (HasAuthority())
			MulticastStartClimbing(Target, OverTarget);  // 服务器（含主机）直接广播
		else
			ServerStartClimbing();                      // 客户端发送请求

		return; // 触发攀爬后不再执行 Jump
	}

	// 正常跳跃
	if (GetCharacterMovement()->IsMovingOnGround())
	{
		Jump();
	}
}

bool As1mpleFpsCharacter::CanClimbing(FVector& OutTarget, FVector& OutOverTarget)const
{	
	if (!GetWorld())return false;

	const float CapsuleRadius = GetCapsuleComponent()->GetScaledCapsuleRadius();

	FVector Forward = GetControlRotation().Vector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	// 检测起点：抬高 + 向前偏移胶囊半径，避免起点落在角色或墙内部
	FVector Start = GetActorLocation();
	Start.Z += 50.0f;
	Start += Forward * CapsuleRadius;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	// 用简单碰撞(盒子/凸包)而非复杂碰撞：复杂碰撞依赖静态网格勾选 Allow CPU Access，
	// 很多商城/Paragon 网格默认没开，导致射线打不到、这些墙爬不上去。
	Params.bTraceComplex = false;

	struct FClimbCandidate
	{
		FHitResult WallHit;
		float AngleDeg;
	};
	TArray<FClimbCandidate> Candidates;
	for (float Degree = -SampleAngleRange * 0.5;Degree <= SampleAngleRange * 0.5;Degree += SampleAngleStep) {
		FRotator Rot = FRotator(0.f, Degree, 0.f);
		FVector Dir = Rot.RotateVector(Forward);
		FVector End = Start + Dir * ClimbCheckDistance;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params)) {
			FVector WallNormal = Hit.ImpactNormal;
			WallNormal.Z = 0.f;
			WallNormal.Normalize();

			float DotProduct = FVector::DotProduct(Forward, WallNormal);
			float RealDotProduct = FMath::Acos(FMath::Clamp(-DotProduct,-1.0f,1.0f));
			float Angle = FMath::RadiansToDegrees(RealDotProduct);

			if (Angle <= MaxAngleToWall) {
				FClimbCandidate Candidate;
				Candidate.WallHit = Hit;
				Candidate.AngleDeg = Angle;
				Candidates.Add(Candidate);
			}
		}
	}
	if (Candidates.Num() <= 0) {
		return false;
	}

	Candidates.Sort([](const FClimbCandidate& A, const FClimbCandidate& B) {
		return A.AngleDeg < B.AngleDeg;
		});
	FHitResult BestHit = Candidates[0].WallHit;

	// 顶部检测：从墙顶上方足够高处向下发射，命中「朝上」的顶面。
	// 不能直接用墙面撞击点的 XY——它正好落在墙正面(边界)上，向下射线可能擦着墙外漏过去导致「墙顶检测失败」。
	// 沿墙面法线往墙里偏移一点，保证射线落在墙的顶面范围内。
	FVector WallInto = -BestHit.ImpactNormal;
	WallInto.Z = 0.0f;
	WallInto.Normalize();
	if (WallInto.IsNearlyZero()) WallInto = Forward;

	FVector AboveWall = BestHit.ImpactPoint + WallInto * 10.0f;
	AboveWall.Z = Start.Z + MaxClimbHeight + 50.0f;   // 保证高于任何可攀爬墙顶
	FVector BelowWall = BestHit.ImpactPoint + WallInto * 10.0f;
	BelowWall.Z -= 50.0f;                              // 略低于墙面撞击点
	FHitResult TopHit;
	if (!GetWorld()->LineTraceSingleByChannel(TopHit, AboveWall, BelowWall, ECC_WorldStatic,Params)) {
		return false;
	}

	// 命中的必须是「朝上的顶面」。若法线接近水平，说明打到了墙面/斜面（比如太高翻不过去的实心墙），直接拒绝
	if (TopHit.ImpactNormal.Z < MinTopFaceNormalZ) {
		return false;
	}

	// 计算落点：胶囊中心落到墙顶上方，并向墙内偏移「胶囊半径 + 余量」，让整根胶囊都压在墙顶平面上
	const float CapsuleHalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	FVector IntoWall = -BestHit.ImpactNormal;   // ImpactNormal 指向玩家，取反才指向墙内
	IntoWall.Z = 0.0f;
	IntoWall.Normalize();
	if (IntoWall.IsNearlyZero()) IntoWall = Forward;

	FVector Target = TopHit.ImpactPoint;        // 墙顶前缘
	Target.Z += CapsuleHalfHeight;
	Target += IntoWall * (CapsuleRadius + 10.0f);

	// 越过墙沿的中间点：胶囊保持在墙正面之外，底部抬到比墙顶略高，第二阶段水平移动时才不会扫到墙
	FVector OverTarget = TopHit.ImpactPoint;
	OverTarget.Z += CapsuleHalfHeight + 10.0f;
	OverTarget += (-IntoWall) * (CapsuleRadius + 5.0f);

	// 高度差校验
	float HeightDiff = Target.Z - GetActorLocation().Z;
	if (HeightDiff < MinClimbHeight || HeightDiff > MaxClimbHeight) {
		return false;
	}

	// 落脚点可站立性校验：在落点下方用胶囊 Sweep，确认有可站立支撑且平面够平
	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
	FHitResult SweepHit;
	const FVector SweepStart = Target + FVector(0, 0, 20.0f);
	const FVector SweepEnd = Target - FVector(0, 0, 20.0f);
	if (!GetWorld()->SweepSingleByChannel(SweepHit, SweepStart, SweepEnd, FQuat::Identity,
	                                      ECC_WorldStatic, CapsuleShape, Params))
	{
		return false; // 落点下方无支撑
	}
	if (SweepHit.ImpactNormal.Z < MinTopFaceNormalZ)
	{
		return false; // 支撑面不是水平面
	}

	OutTarget = Target;
	OutOverTarget = OverTarget;
	return true;
}

void As1mpleFpsCharacter::MulticastStartClimbing_Implementation(FVector Target, FVector OverTarget)
{
	if (bIsClimbing) return; // 防止重复触发

	bIsClimbing = true;
	bClimbRequestPending = false;
	StartClimbLocation = GetActorLocation();
	TargetClimbLocation = Target;
	ClimbOverLocation = OverTarget;
	ClimbElapsedTime = 0.0f;

	// 按高度差选动画档位（分界线 ClimbAnimSplitHeight，蓝图可调）
	bClimbIsHigh = (TargetClimbLocation.Z - StartClimbLocation.Z) > ClimbAnimSplitHeight;

	UAnimMontage* FPPMontage = GetFPPClimbMontage();
	UAnimMontage* TPPMontage = GetTPPClimbMontage();

	// 攀爬总时长：至少跟动画一样长，否则移动先结束、动画被提前停掉（只看到一闪）
	const float BaseDuration = FVector::Dist(StartClimbLocation, TargetClimbLocation) / ClimbSpeed;
	ClimbDuration = FMath::Max(BaseDuration, 0.01f);
	if (TPPMontage)
		ClimbDuration = FMath::Max(ClimbDuration, TPPMontage->GetPlayLength());
	if (FPPMontage)
		ClimbDuration = FMath::Max(ClimbDuration, FPPMontage->GetPlayLength());

	// 冻结移动
	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// 播放动画：本地控制者按视角播，其他端只播第三人称全身
	if (IsLocallyControlled())
	{
		if (bIsThirdPerson && TPPMontage)
		{
			PlayThirdPersonMontage(TPPMontage);
		}
		else if (FPPMontage && Mesh1P)
		{
			if (UAnimInstance* FPPAnim = Mesh1P->GetAnimInstance())
			{
				FPPAnim->Montage_Play(FPPMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
			}
		}
	}
	else if (TPPMontage)
	{
		PlayThirdPersonMontage(TPPMontage);
	}
}

void As1mpleFpsCharacter::StopClimbing(bool bCompleted)
{
	if (!bIsClimbing) return;

	bIsClimbing = false;
	bClimbRequestPending = false;

	// 恢复移动模式
	if (bCompleted)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}

	// 停止攀爬动画（按当前档位）
	UAnimMontage* FPPMontage = GetFPPClimbMontage();
	UAnimMontage* TPPMontage = GetTPPClimbMontage();
	if (IsLocallyControlled())
	{
		if (Mesh1P && FPPMontage && Mesh1P->GetAnimInstance())
			Mesh1P->GetAnimInstance()->Montage_Stop(0.2f, FPPMontage);
		if (bIsThirdPerson && GetMesh() && TPPMontage && GetMesh()->GetAnimInstance())
			GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, TPPMontage);
	}
	else
	{
		if (GetMesh() && TPPMontage && GetMesh()->GetAnimInstance())
			GetMesh()->GetAnimInstance()->Montage_Stop(0.2f, TPPMontage);
	}
}

void As1mpleFpsCharacter::ServerStartClimbing_Implementation()
{
	// 服务器权威校验
	if (bIsClimbing || bClimbRequestPending) return;
	if (IsDead()) return;
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp) return;

	// 请求冷却，防止刷请求
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime - LastClimbAttemptTime < 0.5f)
	{
		ClientClimbDenied();
		return;
	}
	LastClimbAttemptTime = CurrentTime;

	// 服务器重新检测，不信任客户端
	FVector ValidTarget, ValidOverTarget;
	if (CanClimbing(ValidTarget, ValidOverTarget))
	{
		MulticastStartClimbing(ValidTarget, ValidOverTarget);
	}
	else
	{
		ClientClimbDenied();
	}
}

void As1mpleFpsCharacter::ServerCancelClimbing_Implementation()
{
	if (!bIsClimbing) return;
	MulticastStopClimbing(false);
}

void As1mpleFpsCharacter::MulticastStopClimbing_Implementation(bool bCompleted)
{
	StopClimbing(bCompleted);
}

void As1mpleFpsCharacter::ClientClimbDenied_Implementation()
{
	bClimbRequestPending = false;
}
