// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrenadeProjectile.h"
#include "GrenadeProjectile_Frag.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API AGrenadeProjectile_Frag : public AGrenadeProjectile
{
	GENERATED_BODY()
public:
	virtual void Explode()override;
	virtual void MulticastExplosionEffect_Implementation(FVector Location)override;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Explosion")
	TSubclassOf<UCameraShakeBase> ExplosionCameraShake;
private:
	void ApplyRadialDamage();
	void ApplyCameraShake();
};
