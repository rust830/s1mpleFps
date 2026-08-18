// Copyright Epic Games, Inc. All Rights Reserved.

#include "HealthComponent.h"
#include "s1mpleFpsCharacter.h"
#include "WeaponInventoryComponent.h"
#include "DamageComponent.h"
#include "HealthData.h"
#include "TP_WeaponComponent.h"
#include "GrenadeComponent.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsGameMode.h"
#include "s1mpleFpsPvPGameMode.h"
#include "s1mpleFpsPlayerController.h"
#include "HUDWidget.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/TimerHandle.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// 组件必须显式复制，否则移过来的 ReplicatedHealth 等属性和 RPC 都会失效
	SetIsReplicatedByDefault(true);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<As1mpleFpsCharacter>(GetOwner());
	DamageComponent = Character ? Character->DamageComponent : nullptr;
	if (!DamageComponent)
	{
		DamageComponent = GetOwner()->FindComponentByClass<UDamageComponent>();
	}

	// 血量/死亡事件接入：原来在 Character 构造里绑定，现在由 HealthComponent 自己接
	if (DamageComponent)
	{
		DamageComponent->OnDeath.AddDynamic(this, &UHealthComponent::Die);
		DamageComponent->OnDamaged.AddDynamic(this, &UHealthComponent::OnHealthDamaged);
	}

	if (!Character)
	{
		return;
	}

	// 同步权威血量上限，避免初始复制用陈旧的默认值 100
	ReplicatedHealth = DamageComponent ? DamageComponent->MaxHealth : 100.0f;
	ReplicatedMaxHealth = DamageComponent ? DamageComponent->MaxHealth : 100.0f;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UHealthComponent, ReplicatedHealth);
	DOREPLIFETIME(UHealthComponent, ReplicatedMaxHealth);
	DOREPLIFETIME(UHealthComponent, bIsDeadReplicated);
	DOREPLIFETIME(UHealthComponent, HealthAmount);
	DOREPLIFETIME(UHealthComponent, HealthTypes);
}

void UHealthComponent::Die()
{
	if (!Character || !DamageComponent)
	{
		return;
	}

	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (GS && GS->bIsWarmUp)
	{
		// 热身阶段不掉血：将血量恢复到满，防止热身结束后永久无敌
		DamageComponent->CurrentHealth = DamageComponent->MaxHealth;
		ReplicatedHealth = DamageComponent->MaxHealth;
		return;
	}

	if (bIsDead)
	{
		return;
	}
	bIsDead = true;
	bIsDeadReplicated = true;

	// 手雷模式：强制收回，不投掷
	if (Character->GrenadeComponent && Character->GrenadeComponent->bIsEquipped)
	{
		Character->GrenadeComponent->ForceUnequip();
	}

	// 停止开火/换弹，防止 Timer 继续循环
	if (Character->WeaponInventoryComponent->CurrentWeapon)
	{
		Character->WeaponInventoryComponent->CurrentWeapon->StopAutoFire();
		Character->WeaponInventoryComponent->CurrentWeapon->CancelReload();
	}

	// 取消治疗计时器，防止死亡时浪费药品
	GetWorld()->GetTimerManager().ClearTimer(HealingHandle);
	if (bIsHealing)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;
	}
	bIsHealing = false;
	HealingDuration = 0.0f;
	OnHealingStateChanged.Broadcast();

	// === 仅服务端执行（Dedicated 或 Listen Server） ===
	const bool bIsServer = GetWorld() && GetWorld()->GetNetMode() != NM_Client;
	if (bIsServer)
	{
		ReplicatedHealth = DamageComponent->CurrentHealth;

		bool bShouldRespawn = true;
		if (As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>())
		{
			APlayerState* KillerPS = nullptr;
			if (DamageComponent->LastInstigator)
			{
				if (APawn* KillerPawn = Cast<APawn>(DamageComponent->LastInstigator))
				{
					KillerPS = KillerPawn->GetPlayerState();
				}
			}
			GM->OnKill(KillerPS, Character->GetPlayerState());
		}
		else if (As1mpleFpsGameMode* SPGM = GetWorld()->GetAuthGameMode<As1mpleFpsGameMode>())
		{
			// PvE：通知 GameMode 玩家死亡（失败判定），失败则不再复活
			bShouldRespawn = SPGM->OnPlayerDeath();
		}

		// 自动复活计时器（PvE 失败后不再复活）
		if (bShouldRespawn)
		{
			GetWorld()->GetTimerManager().SetTimer(RespawnHandle, this, &UHealthComponent::Respawn, RespawnDelay, false);
		}
	}

	// === 仅本地玩家执行（死亡界面 + 输入模式） ===
	if (Character->IsLocallyControlled())
	{
		APlayerController* PC = Cast<APlayerController>(Character->GetController());
		if (PC)
		{
			FInputModeUIOnly InputMode;
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = true;
		}

		// Show death screen（PvE 任务已结束 或 PvP 比赛已结束 时不弹死亡界面，交给结算界面替代）
		As1mpleFpsGameMode* SPGM = GetWorld()->GetAuthGameMode<As1mpleFpsGameMode>();
		const bool bMissionOver = SPGM && (SPGM->bMissionCompleted || SPGM->bMissionFailed);
		const bool bMatchOver = GS && GS->bMatchEnded;
		if (!bMissionOver && !bMatchOver)
		{
			if (DeathScreenWidget)
			{
				DeathScreenWidget->RemoveFromParent();
				DeathScreenWidget = nullptr;
			}
			if (Character->DeathScreenWidgetClass)
			{
				DeathScreenWidget = CreateWidget<UUserWidget>(GetWorld(), Character->DeathScreenWidgetClass);
				if (DeathScreenWidget)
				{
					DeathScreenWidget->AddToViewport(100);
				}
			}
		}
	}

	// === All clients: disable movement + hide mesh (no ragdoll to avoid mesh stretch)
	Character->GetCharacterMovement()->DisableMovement();
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Character->GetMesh()->SetVisibility(false);
	Character->GetMesh()->SetHiddenInGame(true, true);
	Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Character->Mesh1P->SetVisibility(false);
	Character->Mesh1P->SetHiddenInGame(true, true);
	Character->FirstPersonCameraComponent->SetActive(false);
	// 隐藏所有武器
	for (UTP_WeaponComponent* Weapon : Character->WeaponInventoryComponent->WeaponInventory)
	{
		if (Weapon)
		{
			Weapon->SetVisibility(false);
			Weapon->SetHiddenInGame(true, true);
		}
	}
}

void UHealthComponent::Respawn()
{
	if (!Character || !DamageComponent)
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(RespawnHandle);
	GetWorld()->GetTimerManager().ClearTimer(HealingHandle);

	bIsDead = false;
	bIsDeadReplicated = false;
	bIsHealing = false;
	HealingDuration = 0.0f;
	DamageComponent->CurrentHealth = DamageComponent->MaxHealth;
	ReplicatedHealth = DamageComponent->MaxHealth;
	OnHealthChanged.Broadcast(ReplicatedHealth, DamageComponent->MaxHealth);

	// 完全撤销 DisableMovement()：SetActive + MovementMode + PlaneConstraint
	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->SetActive(true);
		MoveComp->SetPlaneConstraintEnabled(true);
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	// 随机重生点（仅服务端设置位置）
	if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
		if (PlayerStarts.Num() > 0)
		{
			int32 RandomIndex = FMath::RandRange(0, PlayerStarts.Num() - 1);
			Character->SetActorLocation(PlayerStarts[RandomIndex]->GetActorLocation());
		}
	}

	// 强制停止所有正在进行的开火/换弹 timer，防止复活后残留
	if (Character->WeaponInventoryComponent->CurrentWeapon)
	{
		Character->WeaponInventoryComponent->CurrentWeapon->StopAutoFire();
	}

	RespawnVisuals();

	// 确定性刷新客户端血条：服务器直接用权威血量推送，不依赖 OnRep_Health / OnRep_bIsDead 的先后顺序
	ClientOnRespawn(ReplicatedHealth, DamageComponent->MaxHealth);
}

void UHealthComponent::RespawnVisuals()
{
	if (!Character)
	{
		return;
	}

	if (DeathScreenWidget)
	{
		DeathScreenWidget->RemoveFromParent();
		DeathScreenWidget = nullptr;
	}

	// 先恢复移动组件（必须在恢复碰撞之前，防止卡几何体）
	UCharacterMovementComponent* MoveComp = Character->GetCharacterMovement();
	if (MoveComp)
	{
		MoveComp->SetActive(true);
		MoveComp->SetPlaneConstraintEnabled(true);
		MoveComp->SetMovementMode(MOVE_Walking);
	}

	Character->GetMesh()->AttachToComponent(Character->GetCapsuleComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	Character->GetMesh()->SetSimulatePhysics(false);
	Character->GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Character->GetMesh()->SetRelativeLocation(Character->DefaultMeshRelativeLocation);
	Character->GetMesh()->SetRelativeRotation(Character->DefaultMeshRelativeRotation);
	Character->GetMesh()->SetOwnerNoSee(true);
	Character->GetMesh()->SetVisibility(true);
	Character->GetMesh()->SetHiddenInGame(false, true);
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Character->Mesh1P->SetVisibility(true);
	Character->Mesh1P->SetHiddenInGame(false, true);
	Character->Mesh1P->SetOnlyOwnerSee(true);
	Character->FirstPersonCameraComponent->SetActive(true);
	// 恢复武器可见性（第三人称仍隐藏）
	if (Character->WeaponInventoryComponent->CurrentWeapon && !Character->bIsThirdPerson)
	{
		Character->WeaponInventoryComponent->CurrentWeapon->SetVisibility(true);
		Character->WeaponInventoryComponent->CurrentWeapon->SetHiddenInGame(false, true);
	}

	if (Character->IsLocallyControlled())
	{
		APlayerController* PC = Cast<APlayerController>(Character->GetController());
		if (PC)
		{
			// 确保复活后控制器仍然 Possess 当前角色
			if (PC->GetPawn() != Character)
			{
				PC->Possess(Character);
			}
			PC->SetInputMode(FInputModeGameOnly());
			PC->bShowMouseCursor = false;
		}
	}
}

void UHealthComponent::HideDeathWidget()
{
	if (DeathScreenWidget)
	{
		DeathScreenWidget->RemoveFromParent();
		DeathScreenWidget = nullptr;
	}
}

void UHealthComponent::OnHealthDamaged(float Damage, AActor* DamageInstigator)
{
	if (!Character || !DamageComponent)
	{
		return;
	}

	ReplicatedHealth = DamageComponent->CurrentHealth;
	OnHealthChanged.Broadcast(ReplicatedHealth, DamageComponent->MaxHealth);

	// 受击屏幕血反馈（红色闪屏，服务器→客户端）
	if (Damage > 0.0f)
	{
		Character->ClientDamageFeedback(0.45f, 0.5f);
	}

	// 受击打断打药：掉血即收手（OnHealthDamaged 仅服务器触发，需同步通知客户端收起）
	if (bIsHealing && Damage > 0.0f)
	{
		CancelHealing();
		ClientCancelHealing();
	}
}

void UHealthComponent::OnRep_Health()
{
	if (DamageComponent)
	{
		DamageComponent->CurrentHealth = ReplicatedHealth;
	}
	OnHealthChanged.Broadcast(ReplicatedHealth, ReplicatedMaxHealth);

	if (Character && Character->IsLocallyControlled())
	{
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->UpdateHealthDisplay(ReplicatedHealth, ReplicatedMaxHealth);
			}
		}
	}
}

void UHealthComponent::OnRep_MaxHealth()
{
	if (DamageComponent)
	{
		DamageComponent->MaxHealth = ReplicatedMaxHealth;
	}
}

void UHealthComponent::OnRep_bIsDead()
{
	if (bIsDeadReplicated && !bIsDead)
	{
		Die();
	}
	else if (!bIsDeadReplicated && bIsDead)
	{
		// 服务端复活 → 客户端恢复视觉效果
		bIsDead = false;

		// 兜底血量用局部变量，绝不要写 ReplicatedHealth 本身，
		// 否则会吞掉随后真正复制过来的 OnRep_Health。
		const float FallbackHealth = ReplicatedMaxHealth;

		if (DamageComponent)
		{
			DamageComponent->CurrentHealth = FallbackHealth;
		}

		OnHealthChanged.Broadcast(FallbackHealth, FallbackHealth);
		if (Character && Character->IsLocallyControlled())
		{
			if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
			{
				if (PC->HUDWidget)
				{
					PC->HUDWidget->UpdateHealthDisplay(FallbackHealth, FallbackHealth);
				}
			}
		}

		RespawnVisuals();
	}
}

void UHealthComponent::ServerRequestRespawn_Implementation()
{
	Respawn();
}

void UHealthComponent::ClientOnRespawn_Implementation(float NewHealth, float NewMaxHealth)
{
	if (DamageComponent)
	{
		DamageComponent->CurrentHealth = NewHealth;
		DamageComponent->MaxHealth = NewMaxHealth;
	}

	OnHealthChanged.Broadcast(NewHealth, NewMaxHealth);

	if (Character && Character->IsLocallyControlled())
	{
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->UpdateHealthDisplay(NewHealth, NewMaxHealth);
			}
		}
	}
}

void UHealthComponent::ServerUseHealth_Implementation(int32 HealthIndex)
{
	UseHealth(HealthIndex);
}

void UHealthComponent::UseHealth(int32 HealthIndex)
{
	if (!Character || !DamageComponent)
	{
		return;
	}

	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		if (bIsHealing) return;
		if (!HealthTypes.IsValidIndex(HealthIndex) || !HealthAmount.IsValidIndex(HealthIndex) || HealthAmount[HealthIndex] <= 0) return;
		if (DamageComponent->CurrentHealth >= DamageComponent->MaxHealth) return;

		UHealthData* Data = HealthTypes[HealthIndex];
		bIsHealing = true;
		HealingDuration = Data->UsingTime;
		OnHealingStateChanged.Broadcast();

		// Slow movement while healing
		SavedWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
		Character->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed * HealingSpeedMultiplier;

		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->UpdateHealingDisplay(true, Data->UsingTime);
			}
		}

		GetWorld()->GetTimerManager().SetTimer(HealingHandle, [this]()
		{
			Character->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;
			bIsHealing = false;
			HealingDuration = 0.0f;
			OnHealingStateChanged.Broadcast();
			if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
			{
				if (PC->HUDWidget)
				{
					PC->HUDWidget->UpdateHealingDisplay(false, 0.0f);
				}
			}
		}, Data->UsingTime, false);

		ServerUseHealth(HealthIndex);
		return;
	}

	if (bIsHealing) return;
	if (!HealthTypes.IsValidIndex(HealthIndex) || !HealthAmount.IsValidIndex(HealthIndex) || HealthAmount[HealthIndex] <= 0) return;
	if (DamageComponent->CurrentHealth >= DamageComponent->MaxHealth) return;

	UHealthData* Data = HealthTypes[HealthIndex];
	bIsHealing = true;
	HealingDuration = Data->UsingTime;
	OnHealingStateChanged.Broadcast();

	// Slow movement while healing (server side)
	SavedWalkSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
	Character->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed * HealingSpeedMultiplier;

	GetWorld()->GetTimerManager().SetTimer(HealingHandle, [this, HealthIndex, Data]()
	{
		if (!HealthAmount.IsValidIndex(HealthIndex)) return;
		Character->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;
		float NewHealth = FMath::Min(DamageComponent->CurrentHealth + Data->HealAmount, DamageComponent->MaxHealth);
		DamageComponent->CurrentHealth = NewHealth;
		ReplicatedHealth = NewHealth;
		OnHealthChanged.Broadcast(ReplicatedHealth, DamageComponent->MaxHealth);

		if (!HealthAmount.IsValidIndex(HealthIndex)) return;
		HealthAmount[HealthIndex]--;
		if (HealthAmount[HealthIndex] <= 0)
		{
			HealthTypes.RemoveAt(HealthIndex);
			HealthAmount.RemoveAt(HealthIndex);
		}
		bIsHealing = false;
		HealingDuration = 0.0f;
		OnHealingStateChanged.Broadcast();
		OnHealthItemsChanged.Broadcast();
	}, Data->UsingTime, false);
}

void UHealthComponent::CancelHealing()
{
	if (!bIsHealing) return;

	GetWorld()->GetTimerManager().ClearTimer(HealingHandle);
	if (Character)
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = SavedWalkSpeed;
	}
	bIsHealing = false;
	HealingDuration = 0.0f;
	OnHealingStateChanged.Broadcast();

	if (Character)
	{
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->UpdateHealingDisplay(false, 0.0f);
			}
		}
	}
}

void UHealthComponent::ServerCancelHealing_Implementation()
{
	CancelHealing();
}

void UHealthComponent::ClientCancelHealing_Implementation()
{
	CancelHealing();
}

void UHealthComponent::OnRep_HealthItems()
{
	OnHealthItemsChanged.Broadcast();
}

void UHealthComponent::GrantHealthItem(UHealthData* HealthDataPtr)
{
	if (!HealthDataPtr) return;
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) return;

	int32 Idx = HealthTypes.Find(HealthDataPtr);
	if (Idx != INDEX_NONE)
	{
		HealthAmount[Idx]++;
	}
	else
	{
		HealthTypes.Add(HealthDataPtr);
		HealthAmount.Add(1);
	}
	OnHealthItemsChanged.Broadcast();
}

void UHealthComponent::OnUseHealth()
{
	// 打药中再按一次 = 取消打药
	if (bIsHealing)
	{
		CancelHealing();
		if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
		{
			ServerCancelHealing();
		}
		return;
	}
	if (HealthTypes.Num() > 0 && HealthAmount.Num() > 0 && HealthAmount[0] > 0)
	{
		UseHealth(0);
	}
}
