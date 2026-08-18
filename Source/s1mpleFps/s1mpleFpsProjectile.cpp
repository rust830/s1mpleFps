// Copyright Epic Games, Inc. All Rights Reserved.

#include "s1mpleFpsProjectile.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DamageComponent.h"
#include "s1mpleFpsCharacter.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"



As1mpleFpsProjectile::As1mpleFpsProjectile() 
{    
	bReplicates = true;
	bNetTemporary = true;
	// 网络带宽优化：子弹短命且命中判定在服务端，降低复制频率与相关距离
	NetUpdateFrequency = 1.0f;
	bAlwaysRelevant = false;
	NetCullDistanceSquared = 5000.0f * 5000.0f; // 约 50m
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->BodyInstance.SetCollisionProfileName("Projectile");
	CollisionComp->OnComponentHit.AddDynamic(this, &As1mpleFpsProjectile::OnHit);		// set up a notification for when this component hits something blocking

	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// 子弹可视化网格
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(CollisionComp);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 3000.f;
	ProjectileMovement->MaxSpeed = 3000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;
}

void As1mpleFpsProjectile::BeginPlay()
{
	Super::BeginPlay();
	ProjectileMovement->ProjectileGravityScale = BulletGravityScale;
	if (BulletTrailEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(BulletTrailEffect, BulletMesh);
	}
}

void As1mpleFpsProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (OtherActor == nullptr || OtherActor == this || OtherActor == GetInstigator())
	{
		return;
	}

	if (OtherComp && OtherComp->IsSimulatingPhysics())
	{
		OtherComp->AddImpulseAtLocation(GetVelocity() * 100.0f, GetActorLocation());
	}

	// 跳过已死亡角色
	if (As1mpleFpsCharacter* HitChar = Cast<As1mpleFpsCharacter>(OtherActor))
	{
		if (HitChar->IsDead()) return;
	}

	
	if (UDamageComponent* Dmg = OtherActor->FindComponentByClass<UDamageComponent>())
	{
		Dmg->ApplyDamage(Hit.BoneName, Damage, ArmorPenetration, GetInstigator(), Hit.Location);
		MulticastHitFeedback(Hit.Location, true);   // 命中角色：广播冒血+命中音
		if (As1mpleFpsCharacter* Player = Cast<As1mpleFpsCharacter>(GetInstigator())) {
			Player->PlayHitMarker(true);
		}
	}
	else
	{
		MulticastHitFeedback(Hit.Location, false);  // 命中墙体：广播火花+撞击音
		if (As1mpleFpsCharacter* Player = Cast<As1mpleFpsCharacter>(GetInstigator())) {
			Player->PlayHitMarker(false);
		}
	}

	Destroy();
}

void As1mpleFpsProjectile::MulticastHitFeedback_Implementation(FVector Location, bool bHitBody)
{
	if (bHitBody)
	{
		if (HitBodyEffect)
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitBodyEffect, Location, FRotator::ZeroRotator);
		if (HitBodySound)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitBodySound, Location);
	}
	else
	{
		if (HitWallEffect)
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), HitWallEffect, Location, FRotator::ZeroRotator);
		if (HitWallSound)
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), HitWallSound, Location);
	}
}
