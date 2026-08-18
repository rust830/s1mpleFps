// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "HealthComponent.generated.h"

class As1mpleFpsCharacter;
class UDamageComponent;
class UHealthData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, Health, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHealthItemsChanged);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class S1MPLEFPS_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	// --- Network replication ---
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable)
	FOnHealthChanged OnHealthChanged;
	UPROPERTY(BlueprintAssignable)
	FOnHealthItemsChanged OnHealthItemsChanged;
	UPROPERTY(BlueprintAssignable)
	FOnHealthItemsChanged OnHealingStateChanged;

	// --- 复制状态（服务器权威） ---
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health)
	float ReplicatedHealth = 100.0f;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth)
	float ReplicatedMaxHealth = 100.0f;
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_bIsDead)
	bool bIsDeadReplicated = false;
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_Health();
	UFUNCTION()
	void OnRep_MaxHealth();
	UFUNCTION()
	void OnRep_bIsDead();

	// --- 死亡 / 重生 / 治疗 ---
	UFUNCTION()
	void Die();
	UFUNCTION()
	void OnHealthDamaged(float Damage, AActor* DamageInstigator);
	UFUNCTION(BlueprintCallable)
	void Respawn();
	void RespawnVisuals();
	UFUNCTION(BlueprintCallable)
	void HideDeathWidget();

	// 服务端复活请求
	UFUNCTION(Server, Reliable)
	void ServerRequestRespawn();

	// 服务端复活后，可靠地把权威血量推送给拥有客户端刷新血条 UI（不依赖 OnRep 回调的先后顺序）
	UFUNCTION(Client, Reliable)
	void ClientOnRespawn(float NewHealth, float NewMaxHealth);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	float RespawnDelay = 5.0f;
	FTimerHandle RespawnHandle;

	// --- 医疗道具（打药） ---
	UPROPERTY(ReplicatedUsing = OnRep_HealthItems, BlueprintReadWrite)
	TArray<UHealthData*> HealthTypes;
	UPROPERTY(ReplicatedUsing = OnRep_HealthItems, BlueprintReadWrite)
	TArray<int32> HealthAmount;

	UFUNCTION()
	void OnRep_HealthItems();
	UFUNCTION(Server, Reliable)
	void ServerUseHealth(int32 HealthIndex);
	UFUNCTION(BlueprintCallable)
	void UseHealth(int32 HealthIndex);
	// 取消打药：恢复移速、清定时器、收起进度环（本地清理，客户端/服务器各自调用）
	UFUNCTION(BlueprintCallable)
	void CancelHealing();
	// 客户端通知服务器取消打药（否则服务器定时器仍会结算回血+扣道具）
	UFUNCTION(Server, Reliable)
	void ServerCancelHealing();
	// 服务器通知客户端取消打药（受击打断时让客户端本地 UI/移速也复位）
	UFUNCTION(Client, Reliable)
	void ClientCancelHealing();

	UPROPERTY(BlueprintReadOnly)
	bool bIsHealing = false;
	UPROPERTY(BlueprintReadOnly)
	float HealingDuration = 0.0f;
	FTimerHandle HealingHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
	float HealingSpeedMultiplier = 0.5f;

	// 道具/治疗入口（Character 的 GrantHealthItem 转发到这里）
	void GrantHealthItem(UHealthData* HealthData);

	// 打药输入回调（Character 的 OnUseHealth 转发到这里）
	void OnUseHealth();

protected:
	virtual void BeginPlay() override;

	// 死亡界面（运行时实例，由本组件创建/销毁）
	UPROPERTY()
	class UUserWidget* DeathScreenWidget;

private:
	UPROPERTY()
	As1mpleFpsCharacter* Character;
	UPROPERTY()
	UDamageComponent* DamageComponent;
	float SavedWalkSpeed;
};
