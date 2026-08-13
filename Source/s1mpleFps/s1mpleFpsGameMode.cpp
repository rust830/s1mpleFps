// Copyright Epic Games, Inc. All Rights Reserved.

#include "s1mpleFpsGameMode.h"
#include "s1mpleFpsCharacter.h"
#include "EnemyCharacter.h"
#include "Engine/World.h"
#include "TimerManager.h"

As1mpleFpsGameMode::As1mpleFpsGameMode()
	: Super()
{
	// DefaultPawnClass is set in Blueprint (Class Defaults → Classes → Default Pawn Class)
}

void As1mpleFpsGameMode::OnKill(AActor* Killer, AActor* Victim)
{
	if (Cast<As1mpleFpsCharacter>(Killer))
	{
		PlayerKills++;
		PlayerScore += 100;
	}

	if (Cast<As1mpleFpsCharacter>(Victim))
	{
		PlayerDeaths++;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Kill: %d | Death: %d | Score: %d | Killer: %s | Victim: %s"),
		PlayerKills, PlayerDeaths, PlayerScore,
		*GetNameSafe(Killer), *GetNameSafe(Victim));

	OnStatsUpdated.Broadcast(PlayerKills, PlayerDeaths, PlayerScore);
}

void As1mpleFpsGameMode::ScheduleEnemyRespawn(TSubclassOf<AEnemyCharacter> EnemyClass, FVector SpawnLoc, FRotator SpawnRot, float Delay)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameMode] Enemy respawn scheduled: %s in %.1fs"), *EnemyClass->GetName(), Delay);
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, [this, EnemyClass, SpawnLoc, SpawnRot]() {
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AEnemyCharacter* NewEnemy = GetWorld()->SpawnActor<AEnemyCharacter>(EnemyClass, SpawnLoc, SpawnRot, Params);
		UE_LOG(LogTemp, Warning, TEXT("[GameMode] Enemy respawned: %s at %s"), *GetNameSafe(NewEnemy), *SpawnLoc.ToString());
	}, Delay, false);
}
