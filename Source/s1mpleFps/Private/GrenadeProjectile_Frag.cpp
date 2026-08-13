// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeProjectile_Frag.h"
#include "GrenadeData.h"
#include "DamageComponent.h"
#include "Engine/OverlapResult.h"
#include "Perception/AISense_Hearing.h"
#include "Kismet/GameplayStatics.h"

void AGrenadeProjectile_Frag::Explode()
{
	if (HasAuthority()) {
		ApplyRadialDamage();
	}
	ApplyCameraShake();
	MulticastExplosionEffect(GetActorLocation());
	Destroy();
}

void AGrenadeProjectile_Frag::MulticastExplosionEffect_Implementation(FVector Location)
{

	if (GrenadeData)
	{
		if (GrenadeData->ExplosionEffect)
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), GrenadeData->ExplosionEffect, Location);
		if (GrenadeData->ExplosionSound)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), GrenadeData->ExplosionSound, Location);
	}
}

void AGrenadeProjectile_Frag::ApplyRadialDamage()
{
	if (!HasAuthority() || !GrenadeData)return;
	UWorld* World = GetWorld();
	FVector Center = GetActorLocation();
	float Radius = GrenadeData->ExplodeRadius;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);
	TSet<AActor*> Set;
	for (const FOverlapResult& Overlap : Overlaps) {
		AActor* Victim = Overlap.GetActor();
		if (!Victim || Set.Contains(Victim))continue;
		Set.Add(Victim);

		FHitResult TraceHit;
		bool bHit = World->LineTraceSingleByChannel(TraceHit, Center, Victim->GetActorLocation(),
			ECC_Visibility, Params);
		// 只有射线击中了障碍物且不是受害者本人才跳过（无遮挡时 bHit==false，正常造成伤害）
		if (bHit && TraceHit.GetActor() != Victim) continue;
		
		UDamageComponent* DamageComponent = Victim->FindComponentByClass<UDamageComponent>();
		if (!DamageComponent) continue;
		float Distance = FVector::Dist(Victim->GetActorLocation(), Center);
		float Damage = GrenadeData->MaxDamage * (1 - (Distance / GrenadeData->ExplodeRadius));
		DamageComponent->ApplyDamage(FName("Grenade"), Damage, GrenadeData->Penetration,
			GetInstigator(), Victim->GetActorLocation());
	}
	UAISense_Hearing::ReportNoiseEvent(World, Center, 1.0f, GetInstigator(),
		Radius * 2.0f, TEXT("Explosion"));

}

void AGrenadeProjectile_Frag::ApplyCameraShake()
{
	if (!HasAuthority())return;
	UWorld* World = GetWorld();
	if (!World || !GrenadeData || !ExplosionCameraShake)return;
	for (FConstPlayerControllerIterator it = World->GetPlayerControllerIterator();it;++it) {
		APlayerController* PC = it->Get();
		if (!PC)continue;
		APawn* Pawn = PC->GetPawn();
		if (!Pawn)continue;
		float Distance = FVector::Dist(Pawn->GetActorLocation(), GetActorLocation());
		if (Distance > GrenadeData->ExplodeRadius)continue;
		float Scale = 1 - (Distance / GrenadeData->ExplodeRadius);

		PC->ClientStartCameraShake(ExplosionCameraShake, Scale);
	}
}
