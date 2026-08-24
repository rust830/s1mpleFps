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
	DOREPLIFETIME(As1mpleFpsCharacter,bIsClimbing);
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
		if (HealthComponent && HealthComponent->bIsDead)
		{
			StopClimbing(false);
		}
		else
		{
			ClimbElapsedTime += DeltaTime;
			float TotalDistance = FVector::Dist(StartClimbLocation, TargetClimbLocation);
			float MoveDuration = TotalDistance / ClimbSpeed;
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
			SetActorLocation(NewLocation, false);

			if (Alpha >= 1.0f)
			{
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
	if (bIsClimbing)return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->NextWeapon();
}

void As1mpleFpsCharacter::PreviousWeapon()
{
	if (bIsClimbing)return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->PreviousWeapon();
}

void As1mpleFpsCharacter::OnWeaponSlot1()
{
	if (bIsClimbing)return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(0);
}

void As1mpleFpsCharacter::OnWeaponSlot2()
{
	if (bIsClimbing)return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(1);
}

void As1mpleFpsCharacter::OnWeaponSlot3()
{
	if (bIsClimbing)return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->SelectWeaponSlot(2);
}

void As1mpleFpsCharacter::OnUseHealth()
{
	if (bIsClimbing)return;
	if (HealthComponent)
	{
		HealthComponent->OnUseHealth();
	}
}

void As1mpleFpsCharacter::Move(const FInputActionValue& Value)
{
	if (bIsClimbing)return;
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
	if (bIsClimbing)return;
	if (WeaponInventoryComponent)
		WeaponInventoryComponent->Interact();
}

void As1mpleFpsCharacter::Reload()
{
	if (bIsClimbing)return;
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
	ApplyHeroVisualNow();
	AppliedHeroIndex = PS->SelectedHeroIndex; // 记录已套用的索引，索引变化时 Tick 会重新套用
	UE_LOG(LogTemplateCharacter, Log, TEXT("[Hero] 服务端套用: idx=%d mesh=%s (%s)"),
		PS->SelectedHeroIndex, *Hero->Mesh->GetName(), *GetName());
}

void As1mpleFpsCharacter::ApplyHeroVisualNow()
{
	if (!SelectedHeroMesh) return;
	GetMesh()->SetSkeletalMesh(SelectedHeroMesh);
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
	if (bIsClimbing)return;
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
	if (HealthComponent && HealthComponent->bIsDead)return;
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp)return;
	if (bIsClimbing) {
		if (GetNetMode() != NM_Client) {
			CancelClimbing();
			return;
		}
		else {
			ServerCancelClimbing();
			return;
		}
	}
	FVector Target;
	FVector OverTarget;
	if (CanClimbing(Target, OverTarget)) {
		if (GetNetMode() != NM_Client) {
			StartClimbing(Target, OverTarget);
		}
		else {
			ServerStartClimbing(Target);
		}
		return; // 触发攀爬后不再执行 Jump
	}
	if (GetCharacterMovement()->IsMovingOnGround()) {
		Jump();
	}
}

bool As1mpleFpsCharacter::CanClimbing(FVector& OutTarget, FVector& OutOverTarget)const
{	
	if (!GetWorld())return false;

	FVector Start = GetActorLocation();
	Start.Z+=50.f;
	FVector Forward = GetControlRotation().Vector();
	Forward.Z = 0.0f;
	Forward.Normalize();

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = true;

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
	if (Candidates.Num() <= 0)return false;

	Candidates.Sort([](const FClimbCandidate& A, const FClimbCandidate& B) {
		return A.AngleDeg < B.AngleDeg;
		});
	FHitResult BestHit = Candidates[0].WallHit;
	FVector AboveWall = BestHit.ImpactPoint + FVector(0, 0, ClimbCheckHeight);
	FVector BelowWall = BestHit.ImpactPoint - FVector(0, 0, ClimbCheckHeight);
	FHitResult TopHit;
	if (!GetWorld()->LineTraceSingleByChannel(TopHit, AboveWall, BelowWall, ECC_WorldStatic,Params))
		return false;

	// 命中的必须是「朝上的顶面」。若法线接近水平，说明打到了墙面/斜面（比如太高翻不过去的实心墙），直接拒绝
	if (TopHit.ImpactNormal.Z < MinTopFaceNormalZ)
		return false;

	// 计算落点：胶囊中心落到墙顶上方，并向墙内偏移「胶囊半径 + 余量」，让整根胶囊都压在墙顶平面上
	float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	float Radius = GetCapsuleComponent()->GetScaledCapsuleRadius();
	FVector IntoWall = -BestHit.ImpactNormal;   // ImpactNormal 指向玩家，取反才指向墙内
	IntoWall.Z = 0.0f;
	IntoWall.Normalize();
	if (IntoWall.IsNearlyZero()) IntoWall = Forward;

	FVector Target = TopHit.ImpactPoint;        // 墙顶前缘
	Target.Z += HalfHeight;
	Target += IntoWall * (Radius + 10.0f);

	// 越过墙沿的中间点：胶囊保持在墙正面之外，底部抬到比墙顶略高，第二阶段水平移动时才不会扫到墙
	FVector OverTarget = TopHit.ImpactPoint;
	OverTarget.Z += HalfHeight + 10.0f;
	OverTarget += (-IntoWall) * (Radius + 5.0f);

	//Height Check
	float HeightDiff = Target.Z - Start.Z;
	if (HeightDiff < MinClimbHeight || HeightDiff > MaxClimbHeight)
		return false;

	OutTarget = Target;
	OutOverTarget = OverTarget;
	return true;
}

void As1mpleFpsCharacter::StartClimbing(const FVector& Target, const FVector& OverTarget)
{
	if (bIsClimbing) return;
	if (GetWorld()->GetNetMode() == NM_Client) return;

	bIsClimbing = true;
	TargetClimbLocation = Target;
	ClimbOverLocation = OverTarget;
	StartClimbLocation = GetActorLocation();
	ClimbElapsedTime = 0.0f;

	GetCharacterMovement()->SetMovementMode(MOVE_None);

	// 第一人称手臂 montage 播在 Mesh1P（仅持有者可见），第三人称全身 montage 广播
	if (FPPClimbMontage && Mesh1P && IsLocallyControlled())
	{
		if (UAnimInstance* FPPAnim = Mesh1P->GetAnimInstance())
		{
			FPPAnim->Montage_Play(FPPClimbMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
		}
	}
	if (TPPClimbMontage)
	{
		MulticastPlayThirdPersonMontage(TPPClimbMontage);
	}
}

void As1mpleFpsCharacter::StopClimbing(bool bCompleted)
{
	if (!bIsClimbing)return;
	bIsClimbing = false;
	if (bCompleted) {
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else {
		GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
}

void As1mpleFpsCharacter::CancelClimbing()
{
	if (GetWorld()->GetNetMode() == NM_Client)
	{
		ServerCancelClimbing();
	}
	else
	{
		StopClimbing(false);
	}
}

void As1mpleFpsCharacter::ServerStartClimbing_Implementation(FVector TargetLocation)
{
	if(bIsClimbing) return;
	if (HealthComponent && HealthComponent->bIsDead) return;
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp) return;

	FVector ValidTarget;
	FVector ValidOverTarget;
	if (CanClimbing(ValidTarget, ValidOverTarget))   // 服务器重新检测，不信任客户端
	{
		StartClimbing(ValidTarget, ValidOverTarget);
	}
}

void As1mpleFpsCharacter::ServerCancelClimbing_Implementation()
{
	StopClimbing(false);
}
