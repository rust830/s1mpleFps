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

	

	OnStatsUpdated.Broadcast(PlayerKills, PlayerDeaths, PlayerScore);

	CheckWinCondition();
}

void As1mpleFpsGameMode::CheckWinCondition()
{
	if (bMissionCompleted || bMissionFailed) return;
	if (PlayerKills >= EnemyKillTarget)
	{
		bMissionCompleted = true;
		
		OnMissionResult.Broadcast(true);
	}
}

bool As1mpleFpsGameMode::OnPlayerDeath()
{
	if (bMissionCompleted || bMissionFailed) return false;
	if (!bFailOnPlayerDeath)
	{
		// 无尽模式：死亡继续复活，仅以击杀目标获胜
		return true;
	}
	bMissionFailed = true;
	
	OnMissionResult.Broadcast(false);
	return false;
}

void As1mpleFpsGameMode::ScheduleEnemyRespawn(TSubclassOf<AEnemyCharacter> EnemyClass, FVector SpawnLoc, FRotator SpawnRot, float Delay)
{
	// 任务结束后不再重生敌人
	if (bMissionCompleted || bMissionFailed) return;

	
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, [this, EnemyClass, SpawnLoc, SpawnRot]() {
		// 世界可能已被销毁（重启游戏 / 退出 PIE）：GetWorld() 为 null 时停止重生，防止空指针崩溃
		UWorld* World = GetWorld();
		if (!World || !EnemyClass) return;

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		AEnemyCharacter* NewEnemy = World->SpawnActor<AEnemyCharacter>(EnemyClass, SpawnLoc, SpawnRot, Params);
		
	}, Delay, false);
}
