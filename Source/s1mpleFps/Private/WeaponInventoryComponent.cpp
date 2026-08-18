// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponInventoryComponent.h"
#include "s1mpleFpsCharacter.h"
#include "TP_WeaponComponent.h"
#include "TP_PickUpComponent.h"
#include "GrenadeComponent.h"
#include "WeaponData.h"
#include "s1mpleFpsPlayerController.h"
#include "HUDWidget.h"
#include "Engine/OverlapResult.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"

UWeaponInventoryComponent::UWeaponInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// 组件必须显式复制，否则 CurrentWeapon/WeaponInventory 等复制属性和 RPC 都会失效
	SetIsReplicatedByDefault(true);
}

void UWeaponInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	Character = Cast<As1mpleFpsCharacter>(GetOwner());
}

void UWeaponInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UWeaponInventoryComponent, CurrentWeapon);
	DOREPLIFETIME_CONDITION(UWeaponInventoryComponent, WeaponInventory, COND_OwnerOnly);
	DOREPLIFETIME(UWeaponInventoryComponent, WeaponIndex);
}

void UWeaponInventoryComponent::SwitchWeapon(int32 Index)
{
	if (!Character)
	{
		return;
	}

	// 手雷模式中：取消（已拉引信则阻止切武器）
	if (Character->GrenadeComponent && Character->GrenadeComponent->bIsEquipped)
	{
		Character->GrenadeComponent->LeaveGrenadeMode();
		if (Character->GrenadeComponent->bIsEquipped)  // LeaveGrenadeMode 失败 = 已拉引信
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

	if (Character->IsLocallyControlled())
	{
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
		{
			if (PC->HUDWidget)
			{
				PC->HUDWidget->BindToWeapon(NewWeapon);
			}
		}
	}
}

void UWeaponInventoryComponent::NextWeapon()
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

void UWeaponInventoryComponent::PreviousWeapon()
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

void UWeaponInventoryComponent::SelectWeaponSlot(int32 Index)
{
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		SwitchWeapon(Index);
		ServerSwitchWeapon(Index);
		return;
	}
	SwitchWeapon(Index);
}

void UWeaponInventoryComponent::Interact()
{
	if (!Character)
	{
		return;
	}

	FVector CameraLocation = Character->FirstPersonCameraComponent->GetComponentLocation();
	FVector CameraForward = Character->FirstPersonCameraComponent->GetForwardVector();

	TArray<FOverlapResult> Overlap;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Character);

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

		if (Character->HasAuthority() && Character->IsLocallyControlled())
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
			if (!Weapon->AttachWeapon(Character)) {

				break;
			}
			WeaponInventory.Add(Weapon);
			SwitchWeapon(WeaponInventory.Num() - 1);
			PickUp->OnPickUp.Broadcast(Character);

			PickUp->bIsAlreadyPickedUp = true;
			HitActor->SetActorEnableCollision(false);

		}
		break;
	}
}

void UWeaponInventoryComponent::Reload()
{
	if (CurrentWeapon) {
		CurrentWeapon->Reload();
	}
	else
	{
	}
}

void UWeaponInventoryComponent::SetActiveWeapon(UTP_WeaponComponent* NewWeapon)
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

void UWeaponInventoryComponent::OnRep_CurrentWeapon()
{
	if (!Character)
	{
		return;
	}

	if (Character->IsLocallyControlled())
	{
		// 本地玩家：武器可能在客户端没 Equip（纯客户端只发 RPC），OnRep 时补 Equip
		if (CurrentWeapon != PreviousClientWeapon)
		{
			if (PreviousClientWeapon)
				PreviousClientWeapon->UnEquip();
			if (CurrentWeapon)
			{
				CurrentWeapon->SetOwningCharacter(Character);
				CurrentWeapon->Equip();
			}
			PreviousClientWeapon = CurrentWeapon;
		}
		if (CurrentWeapon)
		{
			if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Character->GetController()))
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

			CurrentWeapon->SetOwningCharacter(Character);
			CurrentWeapon->Equip();
		}
		PreviousClientWeapon = CurrentWeapon;
	}
}

void UWeaponInventoryComponent::OnRep_WeaponInventory()
{
	if (!Character)
	{
		return;
	}

	if (Character->IsLocallyControlled())
	{
		// Handle pending purchase (ClientPurchaseComplete arrived before replication)
		if (PendingPurchaseIndex >= 0)
		{
			if (WeaponInventory.IsValidIndex(PendingPurchaseIndex) && WeaponInventory[PendingPurchaseIndex])
			{

				UTP_WeaponComponent* Weapon = WeaponInventory[PendingPurchaseIndex];
				Weapon->SetOwningCharacter(Character);
				CurrentWeapon = nullptr;
				SwitchWeapon(PendingPurchaseIndex);
				PendingPurchaseIndex = -1;
				GetWorld()->GetTimerManager().ClearTimer(PurchaseRetryHandle);
			}
			else
			{
				// 动态创建的组件可能尚未复制到达，延迟重试

				if (PurchaseRetryCount < 10)
				{
					PurchaseRetryCount++;
					GetWorld()->GetTimerManager().SetTimer(PurchaseRetryHandle, [this]() {
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
		if (Weapon->GetAttachParent() != Character->Mesh1P)
		{
			Weapon->SetOwningCharacter(Character);
			Weapon->AttachToComponent(Character->Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("GripPoint")));
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
			CurrentWeapon->SetOwningCharacter(Character);
			CurrentWeapon->Equip();
		}
		PreviousClientWeapon = CurrentWeapon;
	}
}

void UWeaponInventoryComponent::ServerPickUpWeapon_Implementation(AActor* HitActor)
{
	if (!Character)
	{
		return;
	}

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
	if (!PickupWeapon->AttachWeapon(Character))
	{
		ClientUndoPickUp(HitActor);
		return;
	}

	WeaponInventory.Add(PickupWeapon);
	SwitchWeapon(WeaponInventory.Num() - 1);

	if (PickUp)
	{
		PickUp->OnPickUp.Broadcast(Character);
		PickUp->bIsAlreadyPickedUp = true;
	}
	HitActor->SetActorEnableCollision(false);
	HitActor->FlushNetDormancy();
	HitActor->ForceNetUpdate();

	MulticastOnPickUp(HitActor);
	ClientSyncWeaponAmmo(WeaponIndex, PickupWeapon->CurrentAmmo, PickupWeapon->SpareAmmo);


}

void UWeaponInventoryComponent::ClientUndoPickUp_Implementation(AActor* HitActor)
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

void UWeaponInventoryComponent::MulticastOnPickUp_Implementation(AActor* HitActor)
{
	if (!Character)
	{
		return;
	}

	if (Character->IsLocallyControlled())
	{

		return;
	}
	if (Character->HasAuthority())
	{

		return;
	}
	if (!HitActor) return;

	UTP_PickUpComponent* PickUp = HitActor->FindComponentByClass<UTP_PickUpComponent>();


	if (PickUp)
	{
		PickUp->bIsAlreadyPickedUp = true;
		PickUp->OnPickUp.Broadcast(Character);
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


		Weapon->AttachToComponent(Character->GetMesh1P(), FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), FName(TEXT("GripPoint")));
		Weapon->ApplyWeaponMeshTransform();
		Weapon->SetOnlyOwnerSee(true);
	}


}

void UWeaponInventoryComponent::ServerSwitchWeapon_Implementation(int32 Index)
{
	SwitchWeapon(Index);
	if (CurrentWeapon)
	{
		ClientSyncWeaponAmmo(WeaponIndex, CurrentWeapon->CurrentAmmo, CurrentWeapon->SpareAmmo);
	}
}

void UWeaponInventoryComponent::ServerPreviousWeapon_Implementation()
{
	PreviousWeapon();
	if (CurrentWeapon)
	{
		ClientSyncWeaponAmmo(WeaponIndex, CurrentWeapon->CurrentAmmo, CurrentWeapon->SpareAmmo);
	}
}

void UWeaponInventoryComponent::ServerNextWeapon_Implementation()
{
	NextWeapon();
	if (CurrentWeapon)
	{
		ClientSyncWeaponAmmo(WeaponIndex, CurrentWeapon->CurrentAmmo, CurrentWeapon->SpareAmmo);
	}
}

void UWeaponInventoryComponent::ClientSyncWeaponAmmo_Implementation(int32 InWeaponIndex, int32 InCurrentAmmo, int32 InSpareAmmo)
{
	if (!WeaponInventory.IsValidIndex(InWeaponIndex)) return;
	UTP_WeaponComponent* Weapon = WeaponInventory[InWeaponIndex];
	if (!Weapon) return;
	Weapon->CurrentAmmo = InCurrentAmmo;
	Weapon->SpareAmmo = InSpareAmmo;
	Weapon->OnAmmoChanged.Broadcast(InCurrentAmmo, InSpareAmmo);
}

void UWeaponInventoryComponent::ServerFireWeapon_Implementation(int32 InWeaponIndex, FVector SpawnLocation, FRotator SpawnRotation)
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

void UWeaponInventoryComponent::ServerReloadWeapon_Implementation(int32 InWeaponIndex)
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

int32 UWeaponInventoryComponent::GrantWeapon(TSubclassOf<UTP_WeaponComponent> WeaponClass, UWeaponData* WeaponDataPtr)
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

	UTP_WeaponComponent* NewWeapon = NewObject<UTP_WeaponComponent>(Character, WeaponClass);
	NewWeapon->SetIsReplicated(true);
	if (WeaponDataPtr)
		NewWeapon->WeaponData = WeaponDataPtr;
	NewWeapon->RegisterComponent();
	NewWeapon->AttachWeapon(Character);

	while (WeaponInventory.Num() <= TargetSlot)
		WeaponInventory.Add(nullptr);

	WeaponInventory[TargetSlot] = NewWeapon;
	SwitchWeapon(TargetSlot);
	ClientSyncWeaponAmmo(WeaponIndex, NewWeapon->CurrentAmmo, NewWeapon->SpareAmmo);
	return TargetSlot;
}

void UWeaponInventoryComponent::RemoveWeaponSlot(int32 RemoveIndex)
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
