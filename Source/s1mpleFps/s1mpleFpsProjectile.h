// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "s1mpleFpsProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraSystem;

UCLASS(config=Game)
class As1mpleFpsProjectile : public AActor
{
	GENERATED_BODY()

	/** Sphere collision component */
	UPROPERTY(VisibleDefaultsOnly, Category=Projectile)
	USphereComponent* CollisionComp;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

public:
	As1mpleFpsProjectile();

	virtual void BeginPlay() override;

	/** called when projectile hits something */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	/** Returns CollisionComp subobject **/
	USphereComponent* GetCollisionComp() const { return CollisionComp; }
	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float Damage = 25.f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float ArmorPenetration = 0.0f;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float BulletGravityScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Feedback")
	USoundBase* HitBodySound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Feedback")
	USoundBase* HitWallSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Feedback")
	UNiagaraSystem* HitBodyEffect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hit Feedback")
	UNiagaraSystem* HitWallEffect;

	// 命中反馈（声音+粒子），服务器广播给所有客户端，保证开枪者也能看到冒血
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHitFeedback(FVector Location, bool bHitBody);

	/** 子弹可视化网格（球体/弹头模型） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* BulletMesh;

	/** 子弹拖尾粒子 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile")
	UParticleSystem* BulletTrailEffect;
};

