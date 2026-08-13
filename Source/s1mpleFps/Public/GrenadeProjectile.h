// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GrenadeProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UGrenadeData;

UCLASS()
class S1MPLEFPS_API AGrenadeProjectile : public AActor
{
	GENERATED_BODY()

public:
	AGrenadeProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> CollisionComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UProjectileMovementComponent> GrenadeMovementComponent;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UStaticMeshComponent> Mesh;

	// GrenadeData 通过 OnRep 触发客户端初始化（解决 BeginPlay 时数据未复制的问题）
	UPROPERTY(ReplicatedUsing = OnRep_GrenadeData, BlueprintReadOnly)
	TObjectPtr<UGrenadeData> GrenadeData;

	UPROPERTY(BlueprintReadOnly)
	float RemainingFuseTime = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bFuseStarted = false;

	bool bInitializedFromData = false;

	// 从 GrenadeData 同步 mesh/物理参数（服务端 + 客户端共用）
	UFUNCTION()
	void OnRep_GrenadeData();
	void InitFromData();
	void StartFusing(float CustomFuseTime = -1.0f);
	FTimerHandle FuseTimerHandle;

protected:
	virtual void BeginPlay() override;

	void OnFuseTimeExpired();
	virtual void Explode();
	UFUNCTION()
	void OnBounce(UPrimitiveComponent* HitComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastExplosionEffect(FVector Location);
	virtual void MulticastExplosionEffect_Implementation(FVector Location);
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	virtual void Tick(float DeltaTime) override;
};
