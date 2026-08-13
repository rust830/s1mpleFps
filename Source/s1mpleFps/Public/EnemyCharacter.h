// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "EnemyCharacter.generated.h"

class UDamageComponent;
class UWidgetComponent;
class UBehaviorTree;
class UWeaponData;
UCLASS()
class S1MPLEFPS_API AEnemyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnemyCharacter();
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	UDamageComponent* DamageComponent;
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly)
	UWidgetComponent* HealthBarWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UBehaviorTree* BehaviourTree;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UWeaponData* EnemyWeapon;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 CurrentAmmo = 30;
	UPROPERTY(EditAnywhere)
	int32 MaxAmmo = 30;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	int32 TotalAmmo = 150;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float OptimalCombatDistance = 5000.f;


	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	bool bIsDead = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool IsFiring = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsStaggered = false;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* FireMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* HitReactMontage;

	UPROPERTY(BlueprintReadOnly)
	float MoveDirection = 0.f;

	FVector OriginalLocation;
	FTimerHandle StaggerHandle;
	FRotator OriginalRotation;

	UFUNCTION()
	void die();
	UFUNCTION()
	void OnDamaged(float Damage, AActor* DamagedInstigator);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastFireEffects(USoundBase* InFireSound, UAnimMontage* InFireMontage);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
