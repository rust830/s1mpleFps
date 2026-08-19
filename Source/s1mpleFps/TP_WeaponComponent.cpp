// Copyright Epic Games, Inc. All Rights Reserved.


#include "TP_WeaponComponent.h"
#include "s1mpleFpsCharacter.h"
#include "WeaponInventoryComponent.h"
#include "s1mpleFpsProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "WeaponData.h"
#include "Animation/AnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "EnemyAIController.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "s1mpleFpsPlayerController.h"
#include "HUDWidget.h"


UTP_WeaponComponent::UTP_WeaponComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
	MuzzleOffset = FVector(100.0f, 0.0f, 10.0f);
	// 武器只在自己（本地控制者）视角可见，敌人视角看不到
	SetOnlyOwnerSee(true);
}


void UTP_WeaponComponent::SwitchFireMode()
{
	if (!WeaponData) return;
	if (!WeaponData->bCanSwitchFireMode) return;
	switch (CurrentFireMode) {
	case EFireModeEnum::SemiAuto:
		CurrentFireMode = EFireModeEnum::Burst; break;
	case EFireModeEnum::Burst:
		CurrentFireMode = EFireModeEnum::FullAuto; break;
	case EFireModeEnum::FullAuto:
		CurrentFireMode = EFireModeEnum::SemiAuto; break;
	}
}

void UTP_WeaponComponent::OnRep_CurrentAmmo()
{
	CurrentAmmo = ReplicatedCurrentAmmo;
	if (!bIsReloading)
		OnAmmoChanged.Broadcast(CurrentAmmo, SpareAmmo);
}

void UTP_WeaponComponent::OnRep_SpareAmmo()
{
	SpareAmmo = ReplicatedSpareAmmo;
	if (!bIsReloading)
		OnAmmoChanged.Broadcast(CurrentAmmo, SpareAmmo);
}

void UTP_WeaponComponent::OnRep_WeaponData()
{
	if (!WeaponData) return;
	// 客户端收到 WeaponData 时补充网格和音效
	if (!GetSkeletalMeshAsset() && WeaponData->WeaponMesh)
		SetSkeletalMeshAsset(WeaponData->WeaponMesh);
	if (!FireSound && WeaponData->FireSound)
		FireSound = WeaponData->FireSound;
	if (!MuzzleFlashEffect && WeaponData->MuzzleFlashEffect)
		MuzzleFlashEffect = WeaponData->MuzzleFlashEffect;
	if (!FireAnimation && WeaponData->FireAnimation)
		FireAnimation = WeaponData->FireAnimation;
	if (!ReloadAnimation && WeaponData->ReloadAnimation)
		ReloadAnimation = WeaponData->ReloadAnimation;
	if (!FireAction) FireAction = WeaponData->WeaponFireAction;
	if (!ReloadAction) ReloadAction = WeaponData->WeaponReloadAction;
	if (!SwitchAction) SwitchAction = WeaponData->WeaponSwitchAction;
	if (!AimAction) AimAction = WeaponData->WeaponAimAction;
	if (!FireMappingContext) FireMappingContext = WeaponData->WeaponMappingContext;

	// WeaponData may replicate after CurrentWeapon; refresh HUD when it arrives.
	if (Character && Character->IsLocallyControlled())
	{
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->Controller))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->UpdateEquipmentDisplay();
			}
		}
	}
}

void UTP_WeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTP_WeaponComponent, ReplicatedCurrentAmmo);
	DOREPLIFETIME(UTP_WeaponComponent, ReplicatedSpareAmmo);
	DOREPLIFETIME(UTP_WeaponComponent, WeaponData);
}

void UTP_WeaponComponent::ServerReload_Implementation()
{
	if (ReplicatedSpareAmmo <= 0) return;
	if (ReplicatedCurrentAmmo >= WeaponData->MaxProjectile) return;

	int32 Needed = WeaponData->MaxProjectile - ReplicatedCurrentAmmo;
	int32 ToReload = FMath::Min(Needed, ReplicatedSpareAmmo);
	ReplicatedCurrentAmmo += ToReload;
	ReplicatedSpareAmmo -= ToReload;
	if (Character && Character->WeaponInventoryComponent) Character->WeaponInventoryComponent->RefreshSlotAmmo(this);
}

void UTP_WeaponComponent::StartAiming()
{
	if (!Character || Character->WeaponInventoryComponent->CurrentWeapon != this) return;
	bIsAiming = true;
	TargetFOV = ADSFOV;
	bHasValidADSTransform = false;

	if (Character->bIsThirdPerson)
	{
		// 第三人称：走弹簧臂，不动武器
		SavedSpringArmLength = Character->ThirdPersonSpringArm->TargetArmLength;
		SavedSocketOffset = Character->ThirdPersonSpringArm->SocketOffset;
		SavedTargetOffset = Character->ThirdPersonSpringArm->TargetOffset;
		Character->ThirdPersonSpringArm->TargetArmLength = ADSSpringArmLength;
		Character->ThirdPersonSpringArm->SocketOffset = ADSSocketOffset;
		Character->ThirdPersonSpringArm->TargetOffset = ADSTargetOffset;
		Character->GetMesh()->SetOwnerNoSee(true);
		return;
	}

	// 第一人称：机瞄相对变换只算一次并缓存（相机与 GripPoint 刚性连接，该变换不随视角变化）
	if (DoesSocketExist(SightAlignSocketName) && DoesSocketExist(MuzzleSocketName))
	{
		CachedADSRelativeTransform = ComputeADSRelativeTransform();
		bHasValidADSTransform = true;
		UE_LOG(LogTemp, Log, TEXT("[ADS] %s 机瞄相对变换缓存成功：位置=%s 旋转=%s"),
			*GetName(),
			*CachedADSRelativeTransform.GetLocation().ToString(),
			*CachedADSRelativeTransform.GetRotation().Rotator().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ADS] 武器 %s 缺少插槽 '%s' 或 '%s'，机瞄对齐失效。请检查枪的骨骼体插槽名，或调整 SightAlignSocketName / MuzzleSocketName。"),
			*GetName(), *SightAlignSocketName.ToString(), *MuzzleSocketName.ToString());
	}
}

void UTP_WeaponComponent::EndAiming()
{
	if (!Character || Character->WeaponInventoryComponent->CurrentWeapon != this) return;
	bIsAiming = false;
	TargetFOV = DefaultFOV;
	if (Character->bIsThirdPerson) {
		Character->ThirdPersonSpringArm->TargetArmLength = SavedSpringArmLength;
		Character->ThirdPersonSpringArm->SocketOffset = SavedSocketOffset;
		Character->ThirdPersonSpringArm->TargetOffset = SavedTargetOffset;
			Character->GetMesh()->SetOwnerNoSee(false);
		}
}

void UTP_WeaponComponent::ToggleAiming()
{
	if (!Character || Character->WeaponInventoryComponent->CurrentWeapon != this) return;
	if (bIsAiming) {
		EndAiming();
	}
	else {
		StartAiming();
	}
}

void UTP_WeaponComponent::Equip()
{
	
	bIsEquipped = true;
	// 武器可见性规则：
	// 1. 本地控制 + 第一人称 → 可见（Mesh1P 的 bOnlyOwnerSee 已限制仅持有者可见）
	// 2. 本地控制 + 第三人称 → 全部隐藏（ReattachWeaponsForView 处理）
	// 3. 远程角色（模拟代理） → 始终隐藏
	const bool bShouldShow = Character && Character->IsLocallyControlled() && !Character->bIsThirdPerson;
	SetVisibility(bShouldShow);
	SetHiddenInGame(!bShouldShow, true);

	CurrentFOV = DefaultFOV;
	TargetFOV = DefaultFOV;
	ADSBlendAlpha = 0.0f;
	CurrentAmmo = ReplicatedCurrentAmmo;
	SpareAmmo = ReplicatedSpareAmmo;
	OnAmmoChanged.Broadcast(CurrentAmmo, SpareAmmo);
		if (WeaponData)
		{
			CurrentFireMode = WeaponData->FireMode;
			CurrentSpread = WeaponData->BaseSpread;
		}

		// 从 WeaponData 补充输入配置（如果组件自身未设置）
	UInputMappingContext* EffectiveMappingContext = FireMappingContext;
	TArray<UInputAction*> EffectiveActions;
	if (WeaponData)
	{
		if (!EffectiveMappingContext)
			EffectiveMappingContext = WeaponData->WeaponMappingContext;
		if (!FireAction) FireAction = WeaponData->WeaponFireAction;
		if (!ReloadAction) ReloadAction = WeaponData->WeaponReloadAction;
		if (!SwitchAction) SwitchAction = WeaponData->WeaponSwitchAction;
		if (!AimAction) AimAction = WeaponData->WeaponAimAction;
	}

	// 记住本次实际使用的映射上下文，供 UnEquip 精确移除。
	// 否则 FireMappingContext 为空时（服务端/主机 OnRep_WeaponData 不触发、不会给它赋值），
	// Equip 加了 WeaponData 的映射上下文，UnEquip 却移除不了，导致武器映射上下文泄漏、武器输入残留。
	FireMappingContext = EffectiveMappingContext;

	if (!Character || !Character->GetController())
	{
		
		return;
	}
	if (!Character->IsLocallyControlled())
	{
		
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (EffectiveMappingContext)
		{
			Subsystem->AddMappingContext(EffectiveMappingContext, 1);
		}
		else
		{
			
		}
	}

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent))
	{
		Input->ClearBindingsForObject(this);
		if (FireAction)
		{
			Input->BindAction(FireAction, ETriggerEvent::Started, this, &UTP_WeaponComponent::StartFire);
			Input->BindAction(FireAction, ETriggerEvent::Completed, this, &UTP_WeaponComponent::StopFire);
			
		}
		else
		{
			
		}
		if (ReloadAction)
		{
			Input->BindAction(ReloadAction, ETriggerEvent::Started, this, &UTP_WeaponComponent::Reload);
		}
		if (SwitchAction)
		{
			Input->BindAction(SwitchAction, ETriggerEvent::Started, this, &UTP_WeaponComponent::SwitchFireMode);
		}
		if (AimAction)
		{
			Input->BindAction(AimAction, ETriggerEvent::Started, this, &UTP_WeaponComponent::ToggleAiming);
		}
	}
}

void UTP_WeaponComponent::UnEquip()
{
	bIsEquipped = false;

	StopAutoFire();
	bIsTriggerHeld = false;
	bIsOnFireCooldown = false;
	bIsReloading = false;
	bIsAiming = false;
	TargetFOV = DefaultFOV;
	ADSBlendAlpha = 0.0f;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
		GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	}

	SetVisibility(false);

	if (!Character || !Character->GetController()) return;

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!PC) return;

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(PC->InputComponent))
	{
		Input->ClearBindingsForObject(this);
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (FireMappingContext)
		{
			Subsystem->RemoveMappingContext(FireMappingContext);
		}
	}
}

void UTP_WeaponComponent::Reload()
{
	if (!Character || Character->WeaponInventoryComponent->CurrentWeapon != this) return;
	if (bIsReloading || !WeaponData) return;
	if (CurrentAmmo >= WeaponData->MaxProjectile) return;
	if (SpareAmmo <= 0) return;

	bIsReloading = true;

	// 在武器自身骨骼网格上播放换弹动画序列
	PlayWeaponAnimation(ReloadAnimation, false);

	if (Character->HasAuthority())
	{
		int32 Needed = WeaponData->MaxProjectile - ReplicatedCurrentAmmo;
		PendingReloadAmount = FMath::Min(Needed, ReplicatedSpareAmmo);
		Character->MulticastPlayThirdPersonMontage(Character->ThirdPersonReloadMontage);
	}
	else
	{
		// 乐观预测：立即本地加弹并刷新 HUD。
		// 必须在 Reload 里预测、且靠 OnRep 的 =（覆盖）来对账；绝不能在 FinishReload 里再 +=，
		// 否则会和 OnRep 已覆盖的值叠加成双倍。
		int32 Needed = WeaponData->MaxProjectile - CurrentAmmo;
		PendingReloadAmount = FMath::Min(Needed, SpareAmmo);
		OnAmmoChanged.Broadcast(CurrentAmmo, SpareAmmo);
		Character->WeaponInventoryComponent->ServerReloadWeapon(Character->WeaponInventoryComponent->WeaponIndex);
	}

	GetWorld()->GetTimerManager().SetTimer(ReloadTimerHandle, this, &UTP_WeaponComponent::FinishReload, WeaponData->ReloadTime, false);
}

void UTP_WeaponComponent::FinishReload()
{
	if (Character && Character->HasAuthority())
	{
		ReplicatedCurrentAmmo += PendingReloadAmount;
		ReplicatedSpareAmmo -= PendingReloadAmount;
		CurrentAmmo = ReplicatedCurrentAmmo;
		SpareAmmo = ReplicatedSpareAmmo;
		if (Character->WeaponInventoryComponent) Character->WeaponInventoryComponent->RefreshSlotAmmo(this);
	}
	PendingReloadAmount = 0;
	OnAmmoChanged.Broadcast(CurrentAmmo, SpareAmmo);
	bIsReloading = false;
}

void UTP_WeaponComponent::CancelReload()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimerHandle);
	}
	bIsReloading = false;
	PendingReloadAmount = 0;
}

bool UTP_WeaponComponent::bCanFire()
{
	if (bIsOnFireCooldown || bIsReloading) return false;
	return CurrentAmmo > 0;
}

void UTP_WeaponComponent::StartSingleFire()
{
	if (!Character || Character->WeaponInventoryComponent->CurrentWeapon != this)
	{
		
		return;
	}

	if (Character->GetController() == nullptr) return;

	if (Character->IsDead())
	{
		
		return;
	}

	if (!bCanFire())
	{
		
		return;
	}

	

	if (!WeaponData || !WeaponData->ProjectileClass)
	{
		
		return;
	}

	UWorld* const World = GetWorld();
	if (World != nullptr)
	{
		APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
		FRotator SpawnRotation = PlayerController->PlayerCameraManager->GetCameraRotation();
		SpawnRotation.Pitch += FMath::FRandRange(-CurrentSpread, CurrentSpread);
		SpawnRotation.Yaw += FMath::FRandRange(-CurrentSpread, CurrentSpread);
		FVector SpawnLocation;
		if (DoesSocketExist(MuzzleSocketName))
		{
			// 从枪口插槽的真实世界位置射出，子弹/火焰与第一人称枪口对齐
			SpawnLocation = GetSocketLocation(MuzzleSocketName);
		}
		else
		{
			// 兜底：枪没有 muzzle 插槽时退回相机 + 偏移
			SpawnLocation = Character->GetFirstPersonCameraComponent()->GetComponentLocation() + SpawnRotation.RotateVector(MuzzleOffset);
		}

		if (Character->HasAuthority()) {
			ReplicatedCurrentAmmo -= 1;
			if (Character->WeaponInventoryComponent) Character->WeaponInventoryComponent->RefreshSlotAmmo(this);
			PerformFire(SpawnLocation, SpawnRotation);
			Character->MulticastThirdPersonFire(Character->ThirdPersonFireMontage);
			Character->MulticastMuzzleFlash(SpawnLocation, SpawnRotation, MuzzleFlashEffect, FireSound);
		}
		else {
			Character->WeaponInventoryComponent->ServerFireWeapon(Character->WeaponInventoryComponent->WeaponIndex, SpawnLocation, SpawnRotation);
		}
		CurrentAmmo -= 1;
		OnAmmoChanged.Broadcast(CurrentAmmo, SpareAmmo);
		if (MuzzleFlashEffect != nullptr)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlashEffect, SpawnLocation, SpawnRotation);
		}
		float Vertical = FMath::FRandRange(WeaponData->MinVertical, WeaponData->MaxVertical);
		float Horizon = FMath::FRandRange(WeaponData->MinHorizon, WeaponData->MaxHorizon);
		AccumulatedRecoil += FVector2D(Horizon, Vertical);
		CurrentSpread = FMath::Min(CurrentSpread + WeaponData->SpreadPerShot, WeaponData->MaxSpread);
		{
			const float Intensity = WeaponData->MuzzleShakeIntensity;

			MuzzleShakeOffset.X += Intensity * FMath::FRandRange(
				WeaponData->MuzzleShakeXRange.X, WeaponData->MuzzleShakeXRange.Y);
			MuzzleShakeOffset.Y += Intensity * 0.2f * FMath::FRandRange(-1.0f, 0.0f);
			MuzzleShakeOffset.Z += Intensity * FMath::FRandRange(
				WeaponData->MuzzleShakeZRange.X, WeaponData->MuzzleShakeZRange.Y);

			MuzzleShakeRotation.Pitch += Intensity * FMath::FRandRange(
				WeaponData->MuzzleShakePitchRange.X, WeaponData->MuzzleShakePitchRange.Y);
			MuzzleShakeRotation.Yaw += Intensity * FMath::FRandRange(
				WeaponData->MuzzleShakeYawRange.X, WeaponData->MuzzleShakeYawRange.Y);
		}
		if (CurrentFireMode == EFireModeEnum::Burst)
		{
			BurstRemaining--;
			if (BurstRemaining <= 0)
			{
				StopAutoFire();
			}
		}
	}

	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}

	if (FireAnimation != nullptr)
	{
		// 在武器自身骨骼网格上播放开火动画序列
		PlayWeaponAnimation(FireAnimation, false);
	}

	if (CurrentFireMode == EFireModeEnum::SemiAuto) {
		bIsOnFireCooldown = true;
		float CooldownTime = WeaponData ? WeaponData->FireRate : 0.1f;
		GetWorld()->GetTimerManager().SetTimer(FireRateTimerHandle, this, &UTP_WeaponComponent::ResetFireCooldown, CooldownTime, false);
	}

	if (CurrentAmmo <= 0 || bIsReloading) return;

	if (CurrentFireMode == EFireModeEnum::Burst && BurstRemaining > 0)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoFireHandle, this,
			&UTP_WeaponComponent::StartSingleFire,
			WeaponData ? WeaponData->FireRate : 0.1f, false);
	}
	else if (CurrentFireMode == EFireModeEnum::FullAuto && bIsTriggerHeld)
	{
		GetWorld()->GetTimerManager().SetTimer(AutoFireHandle, this,
			&UTP_WeaponComponent::StartSingleFire,
			WeaponData ? WeaponData->FireRate : 0.1f, false);
	}
}

void UTP_WeaponComponent::StopAutoFire()
{
	bIsTriggerHeld = false;
	GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
	bIsOnFireCooldown = false;
	BurstRemaining = 0;
}

void UTP_WeaponComponent::ServerFire_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (!WeaponData)
	{
		
		return;
	}
	if (ReplicatedCurrentAmmo <= 0)
	{
		
		return;
	}
	if (!Character)
	{
		
		return;
	}
	ReplicatedCurrentAmmo -= 1;
	if (Character && Character->WeaponInventoryComponent) Character->WeaponInventoryComponent->RefreshSlotAmmo(this);
	PerformFire(SpawnLocation, SpawnRotation);
	Character->MulticastThirdPersonFire(Character->ThirdPersonFireMontage);
	Character->MulticastMuzzleFlash(SpawnLocation, SpawnRotation, MuzzleFlashEffect, FireSound);
}

void UTP_WeaponComponent::MulticastFireEffect_Implementation(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (Character && Character->IsLocallyControlled()) return;
	if (MuzzleFlashEffect != nullptr && GetWorld()) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlashEffect, SpawnLocation, SpawnRotation);
	}
	if (FireSound != nullptr && Character) {
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, Character->GetActorLocation());
	}
	if (FireAnimation != nullptr) {
		PlayWeaponAnimation(FireAnimation, false);
	}
}

void UTP_WeaponComponent::PerformFire(FVector SpawnLocation, FRotator SpawnRotation)
{
	if (!WeaponData)
	{
		
		return;
	}
	if (!WeaponData->ProjectileClass)
	{
		
		return;
	}
	if (!Character)
	{
		
		return;
	}
	UWorld* const World = GetWorld();
	if (!World) return;
	FActorSpawnParameters ActorSpawnParms;
	ActorSpawnParms.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ActorSpawnParms.Owner = Character;
	ActorSpawnParms.Instigator = Character;
	As1mpleFpsProjectile* Projectile = World->SpawnActor<As1mpleFpsProjectile>(WeaponData->ProjectileClass, SpawnLocation, SpawnRotation, ActorSpawnParms);
	if (Projectile) {
		Projectile->Damage = WeaponData->BaseDamage;
		Projectile->ArmorPenetration = WeaponData->ArmorPenetration;
	}
	else
	{
		
	}
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		Character->GetActorLocation(),
		1.0f,
		Character,
		0.0f,
		TEXT("Gunshot")
	);
}

void UTP_WeaponComponent::StartFire()
{
	
	if (!Character || Character->WeaponInventoryComponent->CurrentWeapon != this)
	{
		
		return;
	}
	if (!Character->GetController()) return;
	if (bIsReloading) return;
	if (CurrentAmmo <= 0)
	{
		
		return;
	}

	bIsTriggerHeld = true;

	switch (CurrentFireMode) {
	case EFireModeEnum::SemiAuto:
		if (bIsOnFireCooldown) return;
		StartSingleFire();
		break;
	case EFireModeEnum::FullAuto:
		bIsOnFireCooldown = false;
		GetWorld()->GetTimerManager().ClearTimer(FireRateTimerHandle);
		StartSingleFire();
		break;
	case EFireModeEnum::Burst:
		if (BurstRemaining <= 0) {
			BurstRemaining = FMath::Min(CurrentAmmo, WeaponData ? WeaponData->BurstCount : 3);
			StartSingleFire();
		}
		break;
	}
}

void UTP_WeaponComponent::StopFire()
{
	bIsTriggerHeld = false;
	if (CurrentFireMode == EFireModeEnum::FullAuto)
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoFireHandle);
	}
}

void UTP_WeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!WeaponData) return;
	if (!Character) return;
	if (Character->WeaponInventoryComponent->CurrentWeapon != this) return;
	if (Character->IsDead()) return;

	ApplyAndDecayRecoil(DeltaTime);
	ApplyAndDecaySpread(DeltaTime);
	DecayMuzzleShake(DeltaTime);
	UpdateWeaponAim(DeltaTime);

	// FOV 插值：使用 Character 自己的 Controller，而非 PlayerIndex 0（多人时客户端拿不到）
	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (PC && PC->GetPawn() == Character)
	{
		APlayerCameraManager* Camera = PC->PlayerCameraManager;
		if (Camera)
		{
			CurrentFOV = FMath::FInterpTo(CurrentFOV, TargetFOV, DeltaTime, AimInterpSpeed);
			Camera->SetFOV(CurrentFOV);
		}
	}
}

void UTP_WeaponComponent::ApplyAndDecayRecoil(float DeltaTime)
{
	if (AccumulatedRecoil.IsNearlyZero()) return;
	FVector2D ToApply = FVector2D::ZeroVector;
	if (WeaponData) {
		float Factor = FMath::Clamp(WeaponData->RecoilRecoverySpeed * DeltaTime, 0.0f, 1.0f);
		ToApply = AccumulatedRecoil * Factor;
		if (Character) {
			if (APlayerController* PC = Cast<APlayerController>(Character->GetController())) {
				PC->AddPitchInput(ToApply.Y);
				PC->AddYawInput(ToApply.X);
			}
		}
	}
	AccumulatedRecoil -= ToApply;
}

void UTP_WeaponComponent::ApplyAndDecaySpread(float DeltaTime)
{
	float EffectiveBase = WeaponData->BaseSpread;
	if (bIsAiming) EffectiveBase *= ADSConcentration;

	if (!bIsTriggerHeld)
	{
		CurrentSpread = FMath::Max(
			CurrentSpread - WeaponData->SpreadRecoverySpeed * DeltaTime,
			EffectiveBase
		);
	}
}

void UTP_WeaponComponent::DecayMuzzleShake(float DeltaTime)
{
	if (MuzzleShakeOffset.IsNearlyZero() && MuzzleShakeRotation.IsNearlyZero())
	{
		return;
	}

	const float DecaySpeed = WeaponData ? WeaponData->MuzzleShakeDecaySpeed : 12.0f;

	MuzzleShakeOffset = FMath::VInterpTo(MuzzleShakeOffset, FVector::ZeroVector, DeltaTime, DecaySpeed);
	MuzzleShakeRotation = FMath::RInterpTo(MuzzleShakeRotation, FRotator::ZeroRotator, DeltaTime, DecaySpeed);

	const FQuat ShakeQuat = MuzzleShakeRotation.Quaternion() * MuzzleOriginalRotation.Quaternion();
	SetRelativeRotation(ShakeQuat);
	SetRelativeLocation(MuzzleOriginalLocation + MuzzleShakeOffset);
}

FTransform UTP_WeaponComponent::ComputeADSRelativeTransform() const
{
	// 不满足条件时返回当前相对变换，避免跳变
	if (!Character || !Character->FirstPersonCameraComponent)
	{
		return GetRelativeTransform();
	}
	if (!DoesSocketExist(SightAlignSocketName) || !DoesSocketExist(MuzzleSocketName))
	{
		return GetRelativeTransform();
	}

	const UCameraComponent* Cam = Character->FirstPersonCameraComponent;
	const FVector CamLoc = Cam->GetComponentLocation();
	const FVector CamFwd = Cam->GetForwardVector();
	const FVector CamUp = Cam->GetUpVector();

	// 枪刚体上的两个固定点（本地空间）
	const FVector LocalSight  = GetSocketTransform(SightAlignSocketName, RTS_Component).GetLocation();
	const FVector LocalMuzzle = GetSocketTransform(MuzzleSocketName, RTS_Component).GetLocation();
	const FVector LocalSightDir = (LocalMuzzle - LocalSight).GetSafeNormal(); // 瞄准轴(朝前)
	if (LocalSightDir.IsNearlyZero())
	{
		return GetRelativeTransform(); // 两插槽重叠，无法确定瞄准轴
	}

	// 用「先对齐瞄准轴、再对齐上方向」两步求旋转，规避单步 FindBetweenNormals 的轴向跳变(乱转)
	FVector LocalUp = FVector::UpVector; // 枪网格的“上”默认 +Z
	LocalUp = (LocalUp - LocalSightDir * (LocalUp | LocalSightDir)).GetSafeNormal();
	if (LocalUp.IsNearlyZero())
	{
		return GetRelativeTransform();
	}
	// 第一步：瞄准轴 -> 相机前向
	const FQuat Q1 = FQuat::FindBetweenNormals(LocalSightDir, CamFwd);
	// 第二步：把“上”绕相机前向滚转到相机上方向（两者都垂直于 CamFwd，不会退化）
	const FQuat Q2 = FQuat::FindBetweenNormals(Q1.RotateVector(LocalUp), CamUp);
	const FQuat RotQ = Q2 * Q1;

	// 照门目标世界位置：相机正前方(准心线上)。枪口与照门共线 => 机瞄直线 = 准心线
	const FVector TargetSightPos = CamLoc + CamFwd * ADSSightDistance + CamUp * ADSSightVerticalOffset;
	const FVector DesiredWorldLoc = TargetSightPos - RotQ.RotateVector(LocalSight);
	const FTransform DesiredWorld(RotQ, DesiredWorldLoc);

	// 转成相对 GripPoint 的相对变换
	if (const USceneComponent* Parent = GetAttachParent())
	{
		const FTransform ParentWorld = Parent->GetSocketTransform(GetAttachSocketName());
		return DesiredWorld.GetRelativeTransform(ParentWorld);
	}
	return DesiredWorld;
}

void UTP_WeaponComponent::UpdateWeaponAim(float DeltaTime)
{
	if (!Character) return;
	// 第三人称走弹簧臂逻辑，不在这里移动武器
	if (Character->bIsThirdPerson) return;

	const float TargetAlpha = bIsAiming ? 1.0f : 0.0f;
	ADSBlendAlpha = FMath::FInterpTo(ADSBlendAlpha, TargetAlpha, DeltaTime, ADSTransformInterpSpeed);
	// 接近端点时直接落地到 0/1，保证稳态时写出的变换逐位一致，不残留每帧极小量插值
	if (FMath::Abs(ADSBlendAlpha - TargetAlpha) < 0.001f)
	{
		ADSBlendAlpha = TargetAlpha;
	}

	// 腰射相对变换 = 基础挂载变换 + 后坐力/枪口抖动（与 DecayMuzzleShake 结果一致）
	FTransform HipRel;
	HipRel.SetLocation(MuzzleOriginalLocation + MuzzleShakeOffset);
	HipRel.SetRotation(MuzzleShakeRotation.Quaternion() * MuzzleOriginalRotation.Quaternion());
	HipRel.SetScale3D(FVector::OneVector);

	// 完全收起：保持腰射变换即可（也兼容 DecayMuzzleShake 已设置的变换）
	if (ADSBlendAlpha <= KINDA_SMALL_NUMBER)
	{
		SetRelativeTransform(HipRel);
		return;
	}

	// 插槽缺失或尚未计算缓存时，保持腰射（仅缩放 FOV）
	if (!bHasValidADSTransform)
	{
		SetRelativeTransform(HipRel);
		return;
	}

	FTransform Blended;
	Blended.Blend(HipRel, CachedADSRelativeTransform, ADSBlendAlpha);
	SetRelativeTransform(Blended);
}

void UTP_WeaponComponent::ResetFireCooldown()
{
	bIsOnFireCooldown = false;
}

void UTP_WeaponComponent::PlayWeaponAnimation(UAnimSequence* Animation, bool bLooping)
{
	if (!Animation) return;
	// 武器自身的骨骼网格（本组件）用单节点动画模式播放，无需 AnimBP
	SetAnimationMode(EAnimationMode::AnimationSingleNode);
	PlayAnimation(Animation, bLooping);
}

void UTP_WeaponComponent::ApplyWeaponMeshTransform()
{
	// AttachToComponent(SnapToTarget) 会把相对变换清零，所以每次挂载后都要按 WeaponData 重新套一遍
	SetRelativeRotation(WeaponData ? WeaponData->WeaponMeshRotation : FRotator::ZeroRotator);
	SetRelativeLocation(WeaponData ? WeaponData->WeaponMeshOffset : FVector::ZeroVector);
}

bool UTP_WeaponComponent::AttachWeapon(As1mpleFpsCharacter* TargetCharacter)
{
	if (TargetCharacter == nullptr)
	{
		return false;
	}

	Character = TargetCharacter;

	if (WeaponData)
	{
		CurrentAmmo = WeaponData->MaxProjectile;
		SpareAmmo = WeaponData->TotalProjectiles;
		ReplicatedCurrentAmmo = WeaponData->MaxProjectile;
		ReplicatedSpareAmmo = WeaponData->TotalProjectiles;
		CurrentFireMode = WeaponData->FireMode;
		CurrentSpread = WeaponData->BaseSpread;
		// 从 WeaponData 补充网格和音效
		if (!GetSkeletalMeshAsset() && WeaponData->WeaponMesh)
			SetSkeletalMeshAsset(WeaponData->WeaponMesh);
		if (!FireSound && WeaponData->FireSound)
			FireSound = WeaponData->FireSound;
		if (!MuzzleFlashEffect && WeaponData->MuzzleFlashEffect)
			MuzzleFlashEffect = WeaponData->MuzzleFlashEffect;
		if (!FireAnimation && WeaponData->FireAnimation)
			FireAnimation = WeaponData->FireAnimation;
		if (!ReloadAnimation && WeaponData->ReloadAnimation)
			ReloadAnimation = WeaponData->ReloadAnimation;
	}

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachmentRules, FName(TEXT("GripPoint")));

	// 武器挂点的相对变换按每把武器的 WeaponData 配置（不同网格轴向/握把位置不同，不能硬编码）
	ApplyWeaponMeshTransform();

	MuzzleOriginalLocation = GetRelativeLocation();
	MuzzleOriginalRotation = GetRelativeRotation();

	SetVisibility(false);
	bIsEquipped = false;

	return true;
}

void UTP_WeaponComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(Character))
	{
		Character->WeaponInventoryComponent->CurrentWeapon = nullptr;

		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}
		}
	}
	Super::EndPlay(EndPlayReason);
}
