// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrenadeProjectile.h"
#include "GrenadeProjectile_Smoke.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API AGrenadeProjectile_Smoke : public AGrenadeProjectile
{
	GENERATED_BODY()
public:
	virtual void Explode()override;
	UFUNCTION(NetMulticast,Reliable)
	void NetMulticastSmokeEffect(FVector Location, float Radius, float Duration,UParticleSystem* Effect);

};
