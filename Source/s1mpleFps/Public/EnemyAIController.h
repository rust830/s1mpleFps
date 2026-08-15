// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Damage.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionTypes.h"

#include "EnemyAIController.generated.h"

/**
 * 
 */

UENUM(BlueprintType)
enum class EAlertLevel :uint8 {
	Calm        UMETA(DisplayName = "Calm"),
	Suspicious  UMETA(DisplayName = "Suspicious"),
	Alert       UMETA(DisplayName = "Alert"),
	Combat     UMETA(DisplayName = "Combat")
};
UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle         UMETA(DisplayName = "Idle"),
	Investigate  UMETA(DisplayName = "Investigate"),
	Combat       UMETA(DisplayName = "Combat"),
	Retreat      UMETA(DisplayName = "Retreat")
};
UENUM(BlueprintType)
enum class ESquatRole : uint8
{
	Assault        UMETA(DisplayName = "Assault"),
	Suppressor  UMETA(DisplayName = "Suppressor"),
	Flanker      UMETA(DisplayName = "Flanker"),

};
UCLASS()
class S1MPLEFPS_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AEnemyAIController();
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UAIPerceptionComponent* PerceptionComponents;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAISenseConfig_Sight* SightConfig;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAISenseConfig_Hearing* HearingConfig;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAISenseConfig_Damage* DamageConfig;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float AlertValue = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float DecaySpeed = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float SightToAlert = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float HearingToAlert = 0.2f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float DamageToAlert = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float SuspiciousThreshold = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float	AlertThreshold = 0.35f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Alertness")
	float CombatThreshold = 0.5f;
	UPROPERTY(BlueprintReadWrite, Category = "AI Squad")
	ESquatRole TeamRole = ESquatRole::Assault;
	UPROPERTY(BlueprintReadWrite, Category = "AI Squad")
	bool bIsLeader = false;
	UPROPERTY(BlueprintReadWrite, Category = "AI Squad")
	UBlackboardComponent* SquadBB = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float FireAccuracy = 0.45f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float ReactionTime = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float ReloadThreshold = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float RetreatHealthThreshold = 0.3f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float RetreatRecoveryThreshold = 0.6f;
	// Health regenerated per second while in Retreat state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float HealthRegenRate = 3.0f;
	// How fast AI rotates to face target (higher = faster turn)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float TurnSpeed = 540.0f;

	// Burst fire: how many shots per burst before pausing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat|Burst")
	int32 BurstShots = 3;
	// Cooldown between bursts (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat|Burst")
	float BurstPause = 2.0f;
	// Spread in degrees when accuracy is 0 (widest cone)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat|Spread")
	float MaxSpreadDegrees = 3.0f;
	// Minimum spread in degrees (best accuracy possible)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat|Spread")
	float MinSpreadDegrees = 0.15f;
	// Spread narrows at close range: spread = base / (1 + distance/DistanceReference)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat|Spread")
	float SpreadDistanceRef = 1500.0f;
	// Offset from actor location to muzzle (used when socket not found)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	FVector MuzzleOffset = FVector(80.f, 0.f, 70.f);
	// Socket name on character mesh for muzzle position
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	FName MuzzleSocketName = "Muzzle";
	// Animation slot for fire montage ("UpperBody" in AnimBP avoids full-body snap)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	FName FireMontageSlot = "DefaultSlot";
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Combat")
	float FireMontageBlendIn = 0.1f;

	int32 CurrentBurstCount = 0;
	float TimeSinceLastBurst = 0.0f;
	float LastFireTime = -1000.f;
	float LastSenseTime_Sight = -1000.f;
	float LastSenseTime_Hearing = -1000.f;
	float LastSenseTime_Damage = -1000.f;
	float TimeEnteredCombat = -1000.f;
	EAIState PreviousAIState = EAIState::Idle;
	FTimerHandle FireAnimTimerHandle;
	FTimerHandle SquadDiscoveryTimerHandle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation;



	UFUNCTION()
	void OnPerceptionUpdated(const TArray<AActor*>& UpdatedActor);
	virtual void OnPossess(APawn* Inpawn) override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	class UBehaviorTree* BTAsset;
	void UpdateBlackboardState();
	UFUNCTION(BlueprintCallable)
	void UpdateAlertValue(float DeltaTime);
	// --- Squad Coordination ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Squad")
	float SquadRadius = 3000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI Squad")
	bool bEnableSquadCoordination = true;

	TArray<TWeakObjectPtr<AEnemyAIController>> SquadMembers;

	void FindSquadMembers();
	void ShareTargetWithSquad();
	void ApplyRoleModifiers();

	UFUNCTION(BlueprintCallable)
	void EnemyFire();
};
