// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyCharacter.h"
#include "EnemyAIController.h"
#include "DamageComponent.h"
#include "Components/WidgetComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BrainComponent.h"
#include "WeaponData.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "s1mpleFpsGameMode.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
// Sets default values
AEnemyCharacter::AEnemyCharacter()
{
 	PrimaryActorTick.bCanEverTick = true;

	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AEnemyAIController::StaticClass();

	DamageComponent = CreateDefaultSubobject<UDamageComponent>(TEXT("DamageComponent"));
	HealthBarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthWidget"));
	HealthBarWidget->SetupAttachment(RootComponent);
	DamageComponent->OnDeath.AddDynamic(this, &AEnemyCharacter::die);
	DamageComponent->OnDamaged.AddDynamic(this, &AEnemyCharacter::OnDamaged);

	// 平滑旋转 + 平滑加减速，避免寻路时动画僵硬
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	bUseControllerRotationYaw = true;
	MoveComp->bOrientRotationToMovement = false;
	MoveComp->bUseControllerDesiredRotation = true; // Controller drives rotation smoothly
	MoveComp->RotationRate = FRotator(0.f, 540.f, 0.f);
		// Rotate skeletal mesh so visual front aligns with actor +X forward
		GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	MoveComp->MaxWalkSpeed = 375.f;
	MoveComp->BrakingDecelerationWalking = 400.f;
}

void AEnemyCharacter::die()
{
	if (bIsDead) return;
	bIsDead = true;

	// 通知 GameMode：统计击杀 + 安排重生
	if (As1mpleFpsGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsGameMode>())
	{
		GM->OnKill(DamageComponent->LastInstigator, this);
		GM->ScheduleEnemyRespawn(GetClass(), OriginalLocation, OriginalRotation, 5.0f);
	}

	// Stop AI
	if (AAIController* AICtrl = Cast<AAIController>(GetController()))
	{
		AICtrl->StopMovement();
		if (AICtrl->BrainComponent)
			{
			AICtrl->BrainComponent->StopLogic(TEXT("Death"));
			}
			AICtrl->UnPossess();
			AICtrl->Destroy();
	}

	// Disable movement and collision
	GetCharacterMovement()->DisableMovement();
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Hide health bar
	if (HealthBarWidget)
	{
		HealthBarWidget->SetVisibility(false);
	}

	// Play death animation or ragdoll
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
	else
	{
		GetMesh()->SetSimulatePhysics(true);
	}

	// 尸体快速清理，新敌人由 GameMode 定时生成
	SetLifeSpan(0.5f);
}

void AEnemyCharacter::OnDamaged(float Damage, AActor* DamagedInstigator)
{
	if (bIsDead || bIsStaggered) return;
	if (HitReactMontage) {
		bIsStaggered = true;
		PlayAnimMontage(HitReactMontage);
		float Duration = HitReactMontage->GetPlayLength();
		GetWorldTimerManager().ClearTimer(StaggerHandle);
		GetWorldTimerManager().SetTimer(StaggerHandle, [this]() {
			this->bIsStaggered = false;
			}, Duration, false);
	}
}

void AEnemyCharacter::MulticastFireEffects_Implementation(USoundBase* InFireSound, UAnimMontage* InFireMontage)
{
	if (InFireSound)
		UGameplayStatics::PlaySoundAtLocation(this, InFireSound, GetActorLocation());

	if (InFireMontage)
	{
		if (UAnimInstance* AnimInst = GetMesh()->GetAnimInstance())
		{
			AnimInst->Montage_Play(InFireMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f, false);
		}
	}
}

// Called when the game starts or when spawned
void AEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	OriginalLocation = GetActorLocation();
	OriginalRotation = GetActorRotation();
	if (EnemyWeapon) {
		OptimalCombatDistance = EnemyWeapon->OptimalCombatDistance;
		TotalAmmo = EnemyWeapon->TotalProjectiles;
		MaxAmmo = EnemyWeapon->MaxProjectile;
		CurrentAmmo = MaxAmmo;
	}

}

// Called every frame
void AEnemyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector Velocity = GetVelocity();
	if (Velocity.SizeSquared() > 1.f)
	{
		FVector Forward = GetActorForwardVector();
		FVector Right = GetActorRightVector();
		float ForwardDot = FVector::DotProduct(Velocity.GetSafeNormal(), Forward);
		float RightDot = FVector::DotProduct(Velocity.GetSafeNormal(), Right);
		MoveDirection = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}
	else
	{
		MoveDirection = 0.f;
	}
}

// Called to bind functionality to input
void AEnemyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}
