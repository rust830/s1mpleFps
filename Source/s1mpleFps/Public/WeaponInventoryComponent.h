// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "WeaponInventoryComponent.generated.h"

class As1mpleFpsCharacter;
class UTP_WeaponComponent;
class UWeaponData;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class S1MPLEFPS_API UWeaponInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponInventoryComponent();

	// --- Network replication ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- 复制状态 ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentWeapon)
	UTP_WeaponComponent* CurrentWeapon;
	UFUNCTION()
	void OnRep_CurrentWeapon();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponInventory)
	TArray<UTP_WeaponComponent*> WeaponInventory;
	UFUNCTION()
	void OnRep_WeaponInventory();

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 WeaponIndex = 0;

	// --- 本地（不复制）状态 ---
	UPROPERTY()
	UTP_WeaponComponent* PreviousClientWeapon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MaxWeaponSlots = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float PickUpDistance = 300.0f;
	int32 PendingPurchaseIndex = -1;
	FTimerHandle PurchaseRetryHandle;
	int32 PurchaseRetryCount = 0;

	// --- 切枪 / 库存 ---
	void SwitchWeapon(int32 Index);
	void NextWeapon();
	void PreviousWeapon();
	void SelectWeaponSlot(int32 Index);
	void SetActiveWeapon(UTP_WeaponComponent* NewWeapon);
	int32 GrantWeapon(TSubclassOf<UTP_WeaponComponent> WeaponClass, UWeaponData* WeaponDataPtr = nullptr);
	void RemoveWeaponSlot(int32 RemoveIndex);

	// --- 拾取 / 换弹 ---
	void Interact();
	void Reload();

	// --- RPC ---
	UFUNCTION(Server, Reliable)
	void ServerSwitchWeapon(int32 Index);
	UFUNCTION(Server, Reliable)
	void ServerPreviousWeapon();
	UFUNCTION(Server, Reliable)
	void ServerNextWeapon();
	UFUNCTION(Server, Reliable)
	void ServerPickUpWeapon(AActor* HitActor);
	UFUNCTION(Client, Reliable)
	void ClientUndoPickUp(AActor* HitActor);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastOnPickUp(AActor* HitActor);
	UFUNCTION(Server, Reliable)
	void ServerFireWeapon(int32 InWeaponIndex, FVector SpawnLocation, FRotator SpawnRotation);
	UFUNCTION(Server, Reliable)
	void ServerReloadWeapon(int32 InWeaponIndex);
	UFUNCTION(Client, Reliable)
	void ClientSyncWeaponAmmo(int32 InWeaponIndex, int32 InCurrentAmmo, int32 InSpareAmmo);

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	As1mpleFpsCharacter* Character;
};
