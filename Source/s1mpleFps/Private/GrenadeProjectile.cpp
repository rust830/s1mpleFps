// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeProjectile.h"
#include "GrenadeData.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AGrenadeProjectile::AGrenadeProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	CollisionComponent->InitSphereRadius(5.0f);
	RootComponent = CollisionComponent;
	GrenadeMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	GrenadeMovementComponent->bShouldBounce = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AGrenadeProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 服务端：SpawnActor 时 GrenadeData 已由 ServerThrowGrenade 设置 → 直接初始化
	// 客户端：此时 GrenadeData 尚未复制到达 → InitFromData 会 return；稍后 OnRep_GrenadeData 补做
	if (GrenadeData)
		InitFromData();

	// 只在服务端启动引信（客户端通过 OnRep_GrenadeData 不会启动引信）
	if (!bFuseStarted && HasAuthority() && GrenadeData)
		StartFusing();
}

// 客户端收到 GrenadeData 复制后，补做 mesh/物理初始化
void AGrenadeProjectile::OnRep_GrenadeData()
{
	InitFromData();
	// 不启动引信 — 引信是服务端权威的
}

// 从 GrenadeData 同步视觉和物理参数，服务端+客户端共用
void AGrenadeProjectile::InitFromData()
{
	// 幂等保护：BeginPlay 和 OnRep_GrenadeData 都会调用本函数，
	// 避免 OnComponentHit 被重复绑定导致 ensure 卡顿
	if (!GrenadeData || bInitializedFromData) return;
	bInitializedFromData = true;
	Mesh->SetStaticMesh(GrenadeData->GrenadeMesh);
	GrenadeMovementComponent->ProjectileGravityScale = GrenadeData->GravityScale;
	GrenadeMovementComponent->bBounceAngleAffectsFriction = true;
	GrenadeMovementComponent->Bounciness = GrenadeData->Bounciness;
	CollisionComponent->OnComponentHit.AddDynamic(this, &AGrenadeProjectile::OnBounce);
}

void AGrenadeProjectile::StartFusing(float CustomFuseTime)
{
	if (!GrenadeData) return;
	if (!HasAuthority()) return;

	// 清除旧计时器 — 允许 ServerThrowGrenade 用 Cooking 剩余时间覆盖 BeginPlay 的默认引信
	if (bFuseStarted)
	{
		GetWorld()->GetTimerManager().ClearTimer(FuseTimerHandle);
		bFuseStarted = false;
	}

	bFuseStarted = true;
	float FuseTime = (CustomFuseTime > 0.0f) ? CustomFuseTime : GrenadeData->FusingTime;
	RemainingFuseTime = FuseTime;
	GetWorld()->GetTimerManager().SetTimer(FuseTimerHandle, this,
		&AGrenadeProjectile::OnFuseTimeExpired, FuseTime, false);
}

void AGrenadeProjectile::OnFuseTimeExpired()
{
	Explode();
}

void AGrenadeProjectile::Explode()
{
	MulticastExplosionEffect(GetActorLocation());
	Destroy();
}

void AGrenadeProjectile::OnBounce(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (GrenadeData && GrenadeData->BounceSound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), GrenadeData->BounceSound,
			Hit.ImpactPoint, NormalImpulse.Size() / GrenadeData->HighThrowSpeed);
	}
}

void AGrenadeProjectile::MulticastExplosionEffect_Implementation(FVector Location)
{
}

void AGrenadeProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGrenadeProjectile, GrenadeData);
}

void AGrenadeProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
