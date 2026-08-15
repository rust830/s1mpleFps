// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeProjectile_Smoke.h"
#include "GrenadeData.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystemComponent.h"
#include "TimerManager.h"

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
            Location, FRotator::ZeroRotator, FVector(Radius / 100.0f), // ��������ƥ��뾶
            true, EPSCPoolMethod::AutoRelease);
        if (PSC)
        {
            PSC->SetFloatParameter(FName("Duration"), Duration);

            // 烟雾持续时间结束后停掉并销毁粒子，否则循环粒子会永远存在
            TWeakObjectPtr<UParticleSystemComponent> WeakPSC(PSC);
            FTimerHandle SmokeTimerHandle;
            GetWorldTimerManager().SetTimer(SmokeTimerHandle, [WeakPSC]()
            {
                if (WeakPSC.IsValid())
                {
                    WeakPSC->Deactivate();
                    WeakPSC->DestroyComponent();
                }
            }, Duration, false);
        }
   
}
