// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeProjectile_Smoke.h"
#include "GrenadeData.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"

void AGrenadeProjectile_Smoke::Explode()
{
	if (!GrenadeData)return;
	NetMulticastSmokeEffect(GetActorLocation(), GrenadeData->SmokeRadius, GrenadeData->SmokeDuration,GrenadeData->ExplosionEffect);
	Destroy();
}

void AGrenadeProjectile_Smoke::NetMulticastSmokeEffect_Implementation(FVector Location, float Radius, float Duration,UParticleSystem* Effect)
{

        UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),Effect,
            Location, FRotator::ZeroRotator, FVector(Radius / 100.0f), // Ëõ·ÅÁ£×ÓÆ¥Åä°ë¾¶
            true, EPSCPoolMethod::AutoRelease);
        if (PSC)
        {
            PSC->SetFloatParameter(FName("Duration"), Duration);
        }
   
}
