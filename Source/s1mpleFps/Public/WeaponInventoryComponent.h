// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "TP_WeaponComponent.h"
#include "WeaponInventoryComponent.generated.h"

class As1mpleFpsCharacter;

// 复制用的「纯数据」武器槽：不复制组件指针（运行时 NewObject 的动态组件复制不可靠），
// 只复制类 + WeaponData + 弹药，客户端收到后在本地自己建组件。
USTRUCT()
struct FWeaponSlotInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TSubclassOf<UTP_WeaponComponent> WeaponClass;

	UPROPERTY()
	UWeaponData* WeaponData = nullptr;

	UPROPERTY()
	int32 CurrentAmmo = 0;

	UPROPERTY()
	int32 SpareAmmo = 0;
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class S1MPLEFPS_API UWeaponInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWeaponInventoryComponent();

	// --- Network replication ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// --- 本地状态（服务端权威持有真组件；客户端据此显示） ---
	UPROPERTY(BlueprintReadOnly)
	UTP_WeaponComponent* CurrentWeapon = nullptr;

	UPROPERTY(BlueprintReadOnly)
	TArray<UTP_WeaponComponent*> WeaponInventory;

	// --- 复制状态：纯数据。服务端改动后 SyncWeaponSlots() 刷新，客户端 OnRep 重建本地组件 ---
	UPROPERTY(ReplicatedUsing = OnRep_WeaponSlots)
	TArray<FWeaponSlotInfo> ReplicatedWeaponSlots;
	UFUNCTION()
	void OnRep_WeaponSlots();

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_WeaponIndex)
	int32 WeaponIndex = 0;
	UFUNCTION()
	void OnRep_WeaponIndex();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	int32 MaxWeaponSlots = 3;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float PickUpDistance = 300.0f;

	// --- 切枪 / 库存 ---
	void SwitchWeapon(int32 Index);
	void NextWeapon();
	void PreviousWeapon();
	void SelectWeaponSlot(int32 Index);
	void SetActiveWeapon(UTP_WeaponComponent* NewWeapon);
	int32 GrantWeapon(TSubclassOf<UTP_WeaponComponent> WeaponClass, UWeaponData* WeaponDataPtr = nullptr);
	void RemoveWeaponSlot(int32 RemoveIndex);

	// --- 数据同步 ---
	// 服务端：把 WeaponInventory 的真实状态（类/WeaponData/弹药）刷进 ReplicatedWeaponSlots（定长 MaxWeaponSlots）
	void SyncWeaponSlots();
	// 服务端：弹药变化后只刷新某一槽的弹药（开火/换弹后调用，避免整表重建）
	void RefreshSlotAmmo(UTP_WeaponComponent* Weapon);
	// 客户端：根据 ReplicatedWeaponSlots 重建/更新本地组件
	void RebuildLocalWeaponsFromSlots();

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
