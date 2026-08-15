// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "DamageComponent.h"
#include "WeaponData.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/EngineTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISense_Damage.h"
#include "Kismet/KismetMathLibrary.h"

void AEnemyAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SquadDiscoveryTimerHandle);
	GetWorldTimerManager().ClearTimer(FireAnimTimerHandle);
	Super::EndPlay(EndPlayReason);
}

AEnemyAIController::AEnemyAIController()
{   //create pc
	PerceptionComponents = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComponents"));

	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->SightRadius = 2500.f;
	SightConfig->LoseSightRadius = 3200.f;
	SightConfig->PeripheralVisionAngleDegrees = 90.f;
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->SetMaxAge(3.5f);
	SightConfig->AutoSuccessRangeFromLastSeenLocation = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 1500.f;
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->SetMaxAge(1.5f);

	DamageConfig = CreateDefaultSubobject<UAISenseConfig_Damage>(TEXT("DamageConfig"));
	DamageConfig->SetMaxAge(3.0f);

	PerceptionComponents->ConfigureSense(*SightConfig);
	PerceptionComponents->ConfigureSense(*HearingConfig);
	PerceptionComponents->ConfigureSense(*DamageConfig);

	PerceptionComponents->OnPerceptionUpdated.AddDynamic(this, &AEnemyAIController::OnPerceptionUpdated);

	UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] Constructor done"));
}

void AEnemyAIController::OnPerceptionUpdated(const TArray<AActor*>& UpdatedActor)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	APawn* MyPawn = GetPawn();
	if (!IsValid(MyPawn)) return;

	AActor* BestTarget = nullptr;
	float BestScore = -1.f;
	bool bBestWasSeen = false;
	bool bBestWasHeard = false;
	bool bBestWasDamaged = false;
	FVector BestHeardLocation = FVector::ZeroVector;
	FVector BestDamageLocation = FVector::ZeroVector;

	for (AActor* Actor : UpdatedActor)
	{
		// Never target ourselves or invalid/destroyed actors
		if (!IsValid(Actor) || Actor == MyPawn || Actor == this)
			continue;

		// 跳过友军（其他敌人）：防止小队自相残杀
		if (Cast<AEnemyCharacter>(Actor))
			continue;

		FActorPerceptionBlueprintInfo Info;
		PerceptionComponents->GetActorsPerception(Actor, Info);

		bool bIsCurrentlySensed = false;
		bool bWasSeen = false;
		bool bWasHeard = false;
		bool bWasDamaged = false;
		FVector HeardLocation = FVector::ZeroVector;
		FVector DamageLocation = FVector::ZeroVector;

		for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
		{
			if (!Stimulus.IsActive())
				continue;

			bIsCurrentlySensed = true;

			if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
			{
				bWasSeen = true;
				LastSenseTime_Sight = GetWorld()->GetTimeSeconds();
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>())
			{
				bWasHeard = true;
				HeardLocation = Stimulus.StimulusLocation;
				LastSenseTime_Hearing = GetWorld()->GetTimeSeconds();
			}
			else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Damage>())
			{
				bWasDamaged = true;
				DamageLocation = Stimulus.StimulusLocation;
				LastSenseTime_Damage = GetWorld()->GetTimeSeconds();
			}
		}

		if (!bIsCurrentlySensed)
			continue;

		float Score = 0.f;
		if (bWasSeen)    Score += 1000.f;
		if (bWasDamaged) Score += 500.f;
		if (bWasHeard)   Score += 100.f;
		Score -= FVector::Dist(GetPawn()->GetActorLocation(), Actor->GetActorLocation()) * 0.1f;

		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Actor;
			bBestWasSeen = bWasSeen;
			bBestWasHeard = bWasHeard;
			bBestWasDamaged = bWasDamaged;
			BestHeardLocation = HeardLocation;
			BestDamageLocation = DamageLocation;
		}
	}

	if (BestTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] >> BestTarget: %s"), *BestTarget->GetName());
		SetFocus(BestTarget, EAIFocusPriority::Gameplay);
		BB->SetValueAsObject("TargetActor", BestTarget);
		if (bBestWasDamaged)
			BB->SetValueAsVector("TargetLocation", BestDamageLocation);
		else if (bBestWasSeen)
			BB->SetValueAsVector("TargetLocation", BestTarget->GetActorLocation());
		else if (bBestWasHeard)
			BB->SetValueAsVector("TargetLocation", BestHeardLocation);

		BB->SetValueAsBool("HasLineOfSight", bBestWasSeen);

		if (bBestWasSeen)
			BB->SetValueAsVector("LastSeenLocation", BestTarget->GetActorLocation());
		if (bBestWasHeard)
			BB->SetValueAsVector("LastHeardLocation", BestHeardLocation);
		if (bBestWasDamaged)
			BB->SetValueAsVector("LastDamageLocation", BestDamageLocation);
	}
	else
	{
		AActor* CurrentTarget = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
		if (CurrentTarget)
		{
			FActorPerceptionBlueprintInfo Info;
			PerceptionComponents->GetActorsPerception(CurrentTarget, Info);
			bool bStillSensed = false;
			for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
			{
				if (Stimulus.IsActive()) { bStillSensed = true; break; }
			}
			if (!bStillSensed)
			{
				UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] >> TargetActor %s lost, clearing"), *CurrentTarget->GetName());
				ClearFocus(EAIFocusPriority::Gameplay);
				BB->ClearValue("TargetActor");
			}
		}
	}
}

void AEnemyAIController::OnPossess(APawn* Inpawn)
{
	Super::OnPossess(Inpawn);
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(Inpawn);
	// 从敌人身上拷贝小队角色（Controller 由 AutoPossess 自动生成，无法在关卡里单独设 TeamRole）
	if (Enemy)
	{
		TeamRole = Enemy->SquadRole;
	}
	UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] OnPossess: %s"), Enemy ? TEXT("EnemyCharacter OK") : TEXT("Cast FAILED"));
	if (Enemy && Enemy->BehaviourTree) {
		RunBehaviorTree(Enemy->BehaviourTree);
		UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] BT started"));
	}

	// 延迟 0.5s 等所有 AI Spawn 完毕再组队
	GetWorldTimerManager().SetTimer(SquadDiscoveryTimerHandle, [this]()
	{
		FindSquadMembers();
		ApplyRoleModifiers();
	}, 0.5f, false);
}

void AEnemyAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateAlertValue(DeltaTime);
	UpdateBlackboardState();

	// Regenerate health in all states except combat
	if (PreviousAIState != EAIState::Combat)
	{
		AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
		if (Enemy && Enemy->DamageComponent)
		{
			Enemy->DamageComponent->CurrentHealth = FMath::Min(
				Enemy->DamageComponent->CurrentHealth + HealthRegenRate * DeltaTime,
				Enemy->DamageComponent->MaxHealth);
		}
	}

	// Rotation is handled by AAIController::UpdateControlRotation (called in Super::Tick)
	// which reads the focus actor set by OnPerceptionUpdated via SetFocus().
	// GetDesiredRotation() automatically uses the focus actor's CURRENT location,
	// so no manual SetFocalPoint or SetControlRotation is needed.

	static float LastDebugLog = -1.f;
	float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastDebugLog > 2.0f)
	{
		UBlackboardComponent* BB = GetBlackboardComponent();
		AActor* Target = BB ? Cast<AActor>(BB->GetValueAsObject("TargetActor")) : nullptr;
		EAIState State = BB ? (EAIState)BB->GetValueAsEnum("CurrentState") : EAIState::Idle;
		UE_LOG(LogTemp, Warning, TEXT("[AI Debug] %s | State: %d | Target: %s | HasPawn: %d"),
			*GetName(), (int32)State, *GetNameSafe(Target), GetPawn() != nullptr);
		LastDebugLog = Now;
	}

	ShareTargetWithSquad();
}

void AEnemyAIController::UpdateBlackboardState()
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (!Enemy) return;

	BB->SetValueAsInt("CurrentAmmo", Enemy->CurrentAmmo);
	BB->SetValueAsInt("TotalAmmo", Enemy->TotalAmmo);

	bool bAmmoLow = (Enemy->CurrentAmmo <= (float)Enemy->MaxAmmo * ReloadThreshold);
	BB->SetValueAsBool("IsAmmoLow", bAmmoLow);

	if (Enemy->DamageComponent)
	{
		BB->SetValueAsFloat("Health", Enemy->DamageComponent->CurrentHealth);
	}

	BB->SetValueAsFloat("OptimalDistance", Enemy->OptimalCombatDistance);

	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
	if (TargetActor)
	{
		FVector EyeLoc = Enemy->GetActorLocation() + Enemy->GetActorForwardVector() * 80.f + FVector(0, 0, 70.f);
		FVector TargetLoc = TargetActor->GetActorLocation() + FVector(0, 0, 70.f);
		FCollisionQueryParams LosParams;
		LosParams.AddIgnoredActor(Enemy);
		FHitResult LosHit;
		GetWorld()->LineTraceSingleByChannel(LosHit, EyeLoc, TargetLoc, ECC_Camera, LosParams);
		bool bHasLos = !LosHit.bBlockingHit || LosHit.GetActor() == TargetActor;

		BB->SetValueAsBool("HasLineOfSight", bHasLos);

		static float LastLosLog = -1.f;
		float Now2 = GetWorld()->GetTimeSeconds();
		if (Now2 - LastLosLog > 0.5f)
		{
			UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] LOS: %s  (blocked by: %s)"),
				bHasLos ? TEXT("CLEAR") : TEXT("BLOCKED"),
				bHasLos ? TEXT("none") : *LosHit.GetActor()->GetName());
			LastLosLog = Now2;
		}

		if (bHasLos)
			BB->SetValueAsVector("TargetLocation", TargetActor->GetActorLocation());
	}
}

void AEnemyAIController::UpdateAlertValue(float DeltaTime)
{
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	const float CurrentTime = GetWorld()->GetTimeSeconds();

	const float SightWindow = 2.0f;
	const float HearingWindow = 1.5f;
	const float DamageWindow = 3.0f;

	bool bHasRecentSight   = (CurrentTime - LastSenseTime_Sight)   < SightWindow;
	bool bHasRecentHearing = (CurrentTime - LastSenseTime_Hearing) < HearingWindow;
	bool bHasRecentDamage  = (CurrentTime - LastSenseTime_Damage)  < DamageWindow;

	if (bHasRecentSight)
	{
		AlertValue += SightToAlert * DeltaTime;
	}
	else if (bHasRecentHearing || bHasRecentDamage)
	{
		AlertValue += (HearingToAlert * 0.1f) * DeltaTime;
	}
	else
	{
		AlertValue -= DecaySpeed * DeltaTime;
	}

	if (bHasRecentHearing && AlertValue < AlertThreshold)
	{
		AlertValue += HearingToAlert;
	}

	if (bHasRecentDamage)
	{
		AlertValue += DamageToAlert;
	}

	AlertValue = FMath::Clamp(AlertValue, 0.0f, 1.0f);

	EAlertLevel NewAlertLevel;
	if (AlertValue >= CombatThreshold)
		NewAlertLevel = EAlertLevel::Combat;
	else if (AlertValue >= AlertThreshold)
		NewAlertLevel = EAlertLevel::Alert;
	else if (AlertValue >= SuspiciousThreshold)
		NewAlertLevel = EAlertLevel::Suspicious;
	else
		NewAlertLevel = EAlertLevel::Calm;

	EAlertLevel OldLevel = (EAlertLevel)BB->GetValueAsEnum("AlertLevel");
	if (OldLevel != NewAlertLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] AlertLevel: %d -> %d (Val=%.2f)"),
			(uint8)OldLevel, (uint8)NewAlertLevel, AlertValue);
	}

	BB->SetValueAsEnum("AlertLevel", (uint8)NewAlertLevel);

	EAIState NewState;
	switch (NewAlertLevel)
	{
	case EAlertLevel::Calm:
		NewState = EAIState::Idle;
		break;
	case EAlertLevel::Suspicious:
	case EAlertLevel::Alert:
		NewState = EAIState::Investigate;
		break;
	case EAlertLevel::Combat:
		NewState = EAIState::Combat;
		break;
	default:
		NewState = EAIState::Idle;
		break;
	}

	// Retreat override: check health/ammo to force retreat even in Combat
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (Enemy && Enemy->DamageComponent)
	{
		float HealthRatio = Enemy->DamageComponent->CurrentHealth / Enemy->DamageComponent->MaxHealth;
		bool bNoAmmoLeft = (Enemy->CurrentAmmo <= 0 && Enemy->TotalAmmo <= 0);

		if (NewState == EAIState::Combat && (HealthRatio < RetreatHealthThreshold || bNoAmmoLeft))
		{
			NewState = EAIState::Retreat;
		}
		else if (PreviousAIState == EAIState::Retreat && HealthRatio > RetreatRecoveryThreshold && !bNoAmmoLeft)
		{
			if (NewState == EAIState::Combat)
			{
				// stay in Combat — health recovered enough to re-engage
			}
		}
	}

	BB->SetValueAsEnum("CurrentState", (uint8)NewState);

	if (NewState == EAIState::Combat && PreviousAIState != EAIState::Combat)
	{
		TimeEnteredCombat = CurrentTime;
		UE_LOG(LogTemp, Warning, TEXT("[AI Ctrl] >> ENTERED COMBAT at t=%.2f"), CurrentTime);
	}
	PreviousAIState = NewState;
}

void AEnemyAIController::EnemyFire()
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (!Enemy) return;
	if (Enemy->bIsStaggered) return;
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;
	AActor* TargetActor = Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
	if (!TargetActor) return;
	UDamageComponent* TargetDmg = TargetActor->FindComponentByClass<UDamageComponent>();
	if (TargetDmg && TargetDmg->CurrentHealth <= 0.0f) return;
	if (GetWorld()->GetTimeSeconds() - TimeEnteredCombat < ReactionTime)
		return;
	FVector ToTarget = (TargetActor->GetActorLocation() - Enemy->GetActorLocation()).GetSafeNormal2D();
	if (FVector::DotProduct(Enemy->GetActorForwardVector().GetSafeNormal2D(), ToTarget) < 0.85f)
		return;
	if (Enemy->CurrentAmmo <= 0) return;
	UWeaponData* Weapon = Enemy->EnemyWeapon;
	if (!Weapon) return;

	float Now = GetWorld()->GetTimeSeconds();

	const float MinShotInterval = 0.12f;
	if (Now - LastFireTime < MinShotInterval)
		return;

	if (CurrentBurstCount >= BurstShots)
	{
		if (Now - TimeSinceLastBurst < BurstPause)
			return;
		CurrentBurstCount = 0;
	}
	if (CurrentBurstCount == 0)
		TimeSinceLastBurst = Now;

	if (Enemy->bIsDead) return;

	FVector MuzzleLocation;
	if (Enemy->GetMesh()->DoesSocketExist(MuzzleSocketName))
	{
		MuzzleLocation = Enemy->GetMesh()->GetSocketLocation(MuzzleSocketName);
	}
	else
	{
		MuzzleLocation = Enemy->GetActorLocation()
			+ Enemy->GetActorForwardVector() * MuzzleOffset.X
			+ Enemy->GetActorRightVector() * MuzzleOffset.Y
			+ Enemy->GetActorUpVector() * MuzzleOffset.Z;
	}
	FVector TargetLocation = TargetActor->GetActorLocation() + FVector(0, 0, 70.f);

	float Distance = FVector::Dist(MuzzleLocation, TargetLocation);
	float DistanceFactor = FMath::Clamp(Distance / SpreadDistanceRef, 0.2f, 2.0f);
	float BaseSpread = FMath::Lerp(MinSpreadDegrees, MaxSpreadDegrees, (1.0f - FireAccuracy) * DistanceFactor);
	float BurstBonus = (CurrentBurstCount == 0) ? -0.3f : 0.0f;
	float SpreadAngle = FMath::Max(0.1f, BaseSpread + BurstBonus);

	FVector PerfectDir = (TargetLocation - MuzzleLocation).GetSafeNormal();
	float AimBlend = FMath::Clamp(FireAccuracy, 0.2f, 0.95f);
	FVector IdealDir = FMath::Lerp(Enemy->GetActorForwardVector(), PerfectDir, AimBlend).GetSafeNormal();

	FVector RightDir = FVector::CrossProduct(IdealDir, FVector::UpVector).GetSafeNormal();
	if (RightDir.IsNearlyZero())
		RightDir = FVector::CrossProduct(IdealDir, FVector::RightVector).GetSafeNormal();
	FVector UpDir = FVector::CrossProduct(RightDir, IdealDir).GetSafeNormal();

	const float RandAngle = FMath::FRandRange(0.0f, UE_TWO_PI);
	const float RandRadius = FMath::Sqrt(FMath::FRandRange(0.0f, 1.0f));
	const float MaxOffset = FMath::Tan(FMath::DegreesToRadians(SpreadAngle));
	FVector SpreadOffset = (RightDir * FMath::Cos(RandAngle) + UpDir * FMath::Sin(RandAngle)) * (RandRadius * MaxOffset);
	FVector FinalDir = (IdealDir + SpreadOffset).GetSafeNormal();
	const FVector TraceEnd = MuzzleLocation + FinalDir * Weapon->MaxDistance;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(Enemy);

	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_Pawn);

	FHitResult Hit;
	bool bHit = GetWorld()->LineTraceSingleByObjectType(Hit, MuzzleLocation, TraceEnd, ObjParams, Params);
	if (bHit)
	{
		// 命中友军（其他敌人）时跳过伤害，避免小队自相残杀；开火音效/动画照常播放
		if (Cast<AEnemyCharacter>(Hit.GetActor()))
		{
			UE_LOG(LogTemp, Warning, TEXT("AI fire blocked by ally: %s"), *Hit.GetActor()->GetName());
		}
		else
		{
			UDamageComponent* VictimDmg = Hit.GetActor()->FindComponentByClass<UDamageComponent>();
			if (VictimDmg)
			{
				float Applied = VictimDmg->ApplyDamage(Hit.BoneName, Weapon->BaseDamage, Weapon->ArmorPenetration, Enemy, Hit.Location);
				UE_LOG(LogTemp, Warning, TEXT("AI HIT %s | bone: %s | dmg: %.1f | HP: %.1f"),
					*Hit.GetActor()->GetName(), *Hit.BoneName.ToString(), Applied, VictimDmg->CurrentHealth);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("AI trace hit non-damageable: %s (use ECC_Pawn or check collision)"), *Hit.GetActor()->GetName());
			}
		}
	}
	else
	{
	}

	UAnimMontage* Montage = FireAnimation ? FireAnimation : Enemy->FireMontage;
	Enemy->MulticastFireEffects(FireSound, Montage);

	// Server-side IsFiring for behavior tree (first shot of burst only)
	if (CurrentBurstCount == 0)
	{
		Enemy->IsFiring = true;
		float Duration = Montage ? Montage->GetPlayLength() : 0.15f;
		GetWorldTimerManager().ClearTimer(FireAnimTimerHandle);
		GetWorldTimerManager().SetTimer(
			FireAnimTimerHandle,
			[Enemy]() { if (Enemy) Enemy->IsFiring = false; },
			Duration,
			false);
	}

	Enemy->CurrentAmmo -= 1;
	CurrentBurstCount++;
	LastFireTime = Now;
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(), MuzzleLocation, 1.0f, Enemy, 0.0f, TEXT("Gunshot"));
}

// ============================================================
// Squad Coordination
// ============================================================

void AEnemyAIController::FindSquadMembers()
{
	if (!bEnableSquadCoordination) return;

	TArray<AActor*> AllControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEnemyAIController::StaticClass(), AllControllers);

	APawn* MyPawn = GetPawn();
	if (!MyPawn) return;

	for (AActor* Actor : AllControllers)
	{
		AEnemyAIController* Other = Cast<AEnemyAIController>(Actor);
		if (!Other || Other == this) continue;
		if (!Other->GetPawn()) continue;

		float Dist = FVector::Dist(MyPawn->GetActorLocation(), Other->GetPawn()->GetActorLocation());
		if (Dist <= SquadRadius)
		{
			SquadMembers.AddUnique(TWeakObjectPtr<AEnemyAIController>(Other));
		}
	}

	if (SquadMembers.Num() > 0 && TeamRole == ESquatRole::Assault)
	{
		bIsLeader = true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AI Squad] %s found %d squad members | Leader=%d | Role=%d"),
		*GetName(), SquadMembers.Num(), bIsLeader, (int32)TeamRole);
}

void AEnemyAIController::ShareTargetWithSquad()
{
	if (!bEnableSquadCoordination || SquadMembers.Num() == 0) return;

	UBlackboardComponent* BB = GetBlackboardComponent();
	if (!BB) return;

	AActor* MyTarget = Cast<AActor>(BB->GetValueAsObject("TargetActor"));
	if (!MyTarget) return;

	// 目标为友军时不共享（防御性检查，正常情况下 OnPerceptionUpdated 已过滤）
	if (Cast<AEnemyCharacter>(MyTarget)) return;

	EAIState MyState = (EAIState)BB->GetValueAsEnum("CurrentState");
	if (MyState != EAIState::Combat) return;

	// 清理已死亡/销毁的队友（弱引用，防止悬空指针导致访问违例崩溃）
	SquadMembers.RemoveAll([](const TWeakObjectPtr<AEnemyAIController>& WeakMember)
	{
		AEnemyAIController* M = WeakMember.Get();
		return !M || !M->GetPawn();
	});

	for (const TWeakObjectPtr<AEnemyAIController>& WeakMember : SquadMembers)
	{
		AEnemyAIController* Member = WeakMember.Get();
		if (!Member) continue;

		UBlackboardComponent* MemberBB = Member->GetBlackboardComponent();
		if (!MemberBB) continue;

		// 共享目标信息
		MemberBB->SetValueAsObject("TargetActor", MyTarget);
		MemberBB->SetValueAsVector("TargetLocation", MyTarget->GetActorLocation());
		MemberBB->SetValueAsVector("LastSeenLocation", MyTarget->GetActorLocation());

		// 强制拉满 AlertValue 到战斗状态
		Member->AlertValue = FMath::Max(Member->AlertValue, CombatThreshold + 0.05f);
	}
}

void AEnemyAIController::ApplyRoleModifiers()
{
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(GetPawn());
	if (!Enemy) return;

	switch (TeamRole)
	{
	case ESquatRole::Assault:
		// 近战突击：冲得最近，反应最快，最顽强
		Enemy->OptimalCombatDistance *= 0.6f;
		FireAccuracy = FMath::Clamp(FireAccuracy * 0.85f, 0.2f, 1.0f);
		ReactionTime *= 0.5f;
		RetreatHealthThreshold = 0.2f;
		break;

	case ESquatRole::Suppressor:
		// 远程压制：站最远，精准点射，弹尽/残血先撤
		Enemy->OptimalCombatDistance *= 1.4f;
		BurstShots = FMath::Max(BurstShots, 6);
		FireAccuracy = FMath::Clamp(FireAccuracy * 1.15f, 0.2f, 1.0f);
		RetreatHealthThreshold = 0.4f;
		break;

	case ESquatRole::Flanker:
		// 侧翼游击：中距离，较快反应
		Enemy->OptimalCombatDistance *= 0.8f;
		ReactionTime *= 0.7f;
		RetreatHealthThreshold = 0.3f;
		break;
	}

	UE_LOG(LogTemp, Warning, TEXT("[AI Squad] Role modifiers applied | %s | Role=%d | Dist=%.0f | Acc=%.2f | React=%.2f"),
		*GetName(), (int32)TeamRole, Enemy->OptimalCombatDistance, FireAccuracy, ReactionTime);
}
