// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeComponent.h"
#include "s1mpleFpsCharacter.h"
#include "TP_WeaponComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"
#include "GrenadeProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "EnhancedInputSubsystems.h"

UGrenadeComponent::UGrenadeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UGrenadeComponent::BeginPlay()
{
	Super::BeginPlay();
	OwnerCharacter = Cast<As1mpleFpsCharacter>(GetOwner());
	if (OwnerCharacter) {
		CachedCamera = OwnerCharacter->GetFirstPersonCameraComponent();
	}
}

void UGrenadeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bIsCooking)return;
	UGrenadeData* Data = GetCurrnetGrenade();
	if (!Data)return;
	CookingElapsed += DeltaTime;
	float Progress = FMath::Clamp(CookingElapsed / Data->FusingTime, 0.0f, 1.0f);
	OnCookingProgressChanged.Broadcast(Progress);
	if (CookingElapsed >= Data->FusingTime) {
		OnCookingExpired();
	}
}

void UGrenadeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UGrenadeComponent, GrenadeTypes);
	DOREPLIFETIME(UGrenadeComponent, GrenadeAmounts);
}

void UGrenadeComponent::ToggleGrenadeMode()
{
	if (!OwnerCharacter)return;
	if (OwnerCharacter->bIsDead)return;
	if (bIsEquipped) {
		if (bIsCooking)return;
		UnequippedGrenade();
	}
	else {
		EquippedGrenade();
	}
}

void UGrenadeComponent::OnStartHighThrowMode()
{
	if (!bIsEquipped || bIsCooking || !HasGrenade())return;
	bIsLowThrow = false;
	OnStartCooking();
}

void UGrenadeComponent::OnStartLowThrowMode()
{
	if (!bIsEquipped || !HasGrenade() || bIsCooking)return;
	bIsLowThrow = true;
	OnStartCooking();
}

void UGrenadeComponent::OnStopThrow()
{
	if (!bIsCooking)return;
	PerformThrowGrenade();
}

bool UGrenadeComponent::HasGrenade() const
{
	return GrenadeTypes.IsValidIndex(CurrentGrenadeIndex)
		&& GrenadeAmounts.IsValidIndex(CurrentGrenadeIndex)
		&& GrenadeAmounts[CurrentGrenadeIndex] > 0;
}

UGrenadeData* UGrenadeComponent::GetCurrnetGrenade()
{
	return HasGrenade() ? GrenadeTypes[CurrentGrenadeIndex] : nullptr;
}

void UGrenadeComponent::AddGrenade(UGrenadeData* Grenade, int32 Amount)
{
	if (!Grenade || Amount <= 0) return;
	int32 FoundIndex = GrenadeTypes.Find(Grenade);
	if (FoundIndex == INDEX_NONE) {
		GrenadeTypes.Add(Grenade);
		GrenadeAmounts.Add(Amount);
	}
	else {
		GrenadeAmounts[FoundIndex] += Amount;
	}
	OnGrenadeInventoryChanged.Broadcast();
}

void UGrenadeComponent::ServerAddGrenade_Implementation(UGrenadeData* Grenade, int32 Amount)
{
	if (!GetOwner()->HasAuthority()) return;
	AddGrenade(Grenade, Amount);
}

void UGrenadeComponent::LeaveGrenadeMode()
{
	if (!bIsEquipped	)return;
	UnequippedGrenade();
}

void UGrenadeComponent::ForceUnequip()
{
	// 死亡时强制收回，无视 bIsCooking
	bIsCooking = false;
	CookingElapsed = 0.0f;
	bIsLowThrow = false;
	RemoveGrenadeMappingContext();
	if (bIsEquipped)
	{
		bIsEquipped = false;
		OnGrenadeEquipped.Broadcast();
		OnCookingProgressChanged.Broadcast(0.0f);
	}
}

void UGrenadeComponent::EquippedGrenade()
{
	if (bIsEquipped || !HasGrenade())return;
	if (!OwnerCharacter || OwnerCharacter->bIsDead)return;

	bIsEquipped = true;
	if (OwnerCharacter->CurrentWeapon)OwnerCharacter->CurrentWeapon->UnEquip();

	AddGrenadeMappingContext();
	UGrenadeData* Data = GetCurrnetGrenade();
	if (Data && Data->EquipSound) {
		UGameplayStatics::PlaySound2D(GetWorld(), Data->EquipSound);
	}
	OnGrenadeEquipped.Broadcast();
}

void UGrenadeComponent::UnequippedGrenade()
{
	if (!bIsEquipped)return;
	bIsEquipped = false;
	bIsCooking = false;
	CookingElapsed = 0.0f;
	bIsLowThrow = false;

	RemoveGrenadeMappingContext();

	if (OwnerCharacter && OwnerCharacter->CurrentWeapon)
		OwnerCharacter->CurrentWeapon->Equip();

	OnGrenadeEquipped.Broadcast();
	OnCookingProgressChanged.Broadcast(0.0f);
}

void UGrenadeComponent::PerformThrowGrenade()
{
	if (!HasGrenade() || !OwnerCharacter) return;
	UGrenadeData* Data = GetCurrnetGrenade();
	if (!Data || !Data->GrenadeProjectileClass)return;
	FVector CamLocation;
	FRotator CamRotation;
	if (CachedCamera)
	{
		CamLocation = CachedCamera->GetComponentLocation();
		CamRotation = CachedCamera->GetComponentRotation();
	}
	else
	{
		CamLocation = OwnerCharacter->GetActorLocation();
		CamRotation = OwnerCharacter->GetActorRotation();
	}

	float Speed = bIsLowThrow ? Data->LowThrowSpeed : Data->HighThrowSpeed;
	float AngleRad = FMath::DegreesToRadians(bIsLowThrow ? Data->LowThrowAngle : Data->HighThrowAngle);

	FVector Forward = CamRotation.Vector();
	FVector Up = FVector::UpVector;
	FVector ThrowDir = Forward * FMath::Cos(AngleRad) + Up * FMath::Sin(AngleRad);
	ThrowDir.Normalize();
	FVector ThrowVelocity = ThrowDir * Speed;
	float RemainingFuse = Data->FusingTime;
	if (bIsCooking && CookingElapsed > 0) {
		RemainingFuse = FMath::Max(RemainingFuse - CookingElapsed, 0.05f);
	}
	ServerThrowGrenade(CurrentGrenadeIndex, ThrowVelocity, RemainingFuse);

	// 投掷完不自动退出投掷模式，只重置烹饪状态，玩家可继续投下一颗或手动退出
	bIsCooking = false;
	CookingElapsed = 0.0f;
	bIsLowThrow = false;
	OnCookingProgressChanged.Broadcast(0.0f);
}

void UGrenadeComponent::OnStartCooking()
{
	bIsCooking = true;
	CookingElapsed = 0.0f;



	UGrenadeData* Data = GetCurrnetGrenade();
	if (Data && Data->PinPullSound)
		UGameplayStatics::PlaySound2D(GetWorld(), Data->PinPullSound);

	OnCookingStart.Broadcast();
}

void UGrenadeComponent::AddGrenadeMappingContext()
{
	if (!IMC_Grenade || !OwnerCharacter) return;
	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (Subsystem)
		Subsystem->AddMappingContext(IMC_Grenade, 1);
}

void UGrenadeComponent::RemoveGrenadeMappingContext()
{

	if (!IMC_Grenade|| !OwnerCharacter) return;
	APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController());
	if (!PC) return;
	ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP) return;
	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
	if (Subsystem)
		Subsystem->RemoveMappingContext(IMC_Grenade);
}

void UGrenadeComponent::NextGrenade()
{
	if (GrenadeTypes.Num() <= 1)return;
	if (bIsCooking)return;
	CurrentGrenadeIndex = (CurrentGrenadeIndex + 1) % (GrenadeTypes.Num());
	OnGrenadeInventoryChanged.Broadcast();
}

void UGrenadeComponent::PrevGrenade()
{
	if (GrenadeTypes.Num() <= 1)return;
	if (bIsCooking)return;
	CurrentGrenadeIndex = (CurrentGrenadeIndex - 1+GrenadeTypes.Num()) % (GrenadeTypes.Num());
	OnGrenadeInventoryChanged.Broadcast();
}

void UGrenadeComponent::OnRep_GrenadeInventory()
{
	// 服务器 RemoveAt 会缩小数组，客户端索引必须夹到有效范围，否则会指空、无法自动切到下一颗
	CurrentGrenadeIndex = FMath::Clamp(CurrentGrenadeIndex, 0, FMath::Max(0, GrenadeTypes.Num() - 1));
	OnGrenadeInventoryChanged.Broadcast();
}

void UGrenadeComponent::NetMulticastThrowSound_Implementation(UGrenadeData* Data, FVector Velocity, float RemainingTime)
{
}

void UGrenadeComponent::OnCookingExpired()
{
	if (!HasGrenade() || !bIsEquipped)return;
	PerformThrowGrenade();
}

void UGrenadeComponent::ServerThrowGrenade_Implementation(int32 GrenadeIndex, FVector Velocity, float RemainingTime)
{
	if (!GetOwner()->HasAuthority() || !OwnerCharacter) return;
	if (!GrenadeTypes.IsValidIndex(GrenadeIndex) || !GrenadeAmounts.IsValidIndex(GrenadeIndex)) return;
	if (GrenadeAmounts[GrenadeIndex] <= 0) return;

	// 关键：不要把 UGrenadeData* 指针直接传过 RPC —— 数据资产指针经网络序列化会变成 NULL。
	// 改为传索引，由服务端用自己的权威 GrenadeTypes 数组查数据资产。
	UGrenadeData* Data = GrenadeTypes[GrenadeIndex];
	if (!Data || !Data->GrenadeProjectileClass) return;

	UWorld* World = GetWorld();
	if (!World)return;

	FVector Forward = OwnerCharacter->GetActorForwardVector();
	FVector Location = OwnerCharacter->GetActorLocation() + Forward * 60.f + FVector(0.f, 0.f, 50.f);

	FActorSpawnParameters Params;
	Params.Owner = OwnerCharacter;
	Params.Instigator = OwnerCharacter;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGrenadeProjectile* Grenade = World->SpawnActor<AGrenadeProjectile>(Data->GrenadeProjectileClass, Location, FRotator::ZeroRotator, Params);
	if (!Grenade) return;

	// 设置数据 + 初速（在 BeginPlay/StartFusing 之前设置，确保客户端 OnRep 后可用）
	Grenade->GrenadeData = Data;
	Grenade->InitFromData();  // 服务端立即应用 mesh/物理参数（OnRep 只在客户端触发）
	if (Grenade->GrenadeMovementComponent)
		Grenade->GrenadeMovementComponent->Velocity = Velocity;
	Grenade->StartFusing(RemainingTime);

	GrenadeAmounts[GrenadeIndex]--;
	if (GrenadeAmounts[GrenadeIndex] <= 0)
	{
		GrenadeTypes.RemoveAt(GrenadeIndex);
		GrenadeAmounts.RemoveAt(GrenadeIndex);
		CurrentGrenadeIndex = FMath::Clamp(CurrentGrenadeIndex, 0, FMath::Max(0, GrenadeTypes.Num() - 1));
	}
	// 服务端(主机)也要广播刷新 HUD；远程客户端靠 OnRep_GrenadeInventory
	OnGrenadeInventoryChanged.Broadcast();
}
