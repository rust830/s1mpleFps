// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GrenadeProjectile.h"
#include "GrenadeProjectile_Flash.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API AGrenadeProjectile_Flash : public AGrenadeProjectile
{
	GENERATED_BODY()
public:
	virtual void Explode() override;
private:
	void ApplyFlashEffect();
};
