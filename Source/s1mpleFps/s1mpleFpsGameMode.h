// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "s1mpleFpsGameMode.generated.h"

class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatsUpdated, int32, Kills, int32, Deaths, int32, Score);

UCLASS(minimalapi)
class As1mpleFpsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	As1mpleFpsGameMode();

	UPROPERTY(BlueprintAssignable)
	FOnStatsUpdated OnStatsUpdated;

	void OnKill(AActor* Killer, AActor* Victim);

	void ScheduleEnemyRespawn(TSubclassOf<AEnemyCharacter> EnemyClass, FVector SpawnLoc, FRotator SpawnRot, float Delay);

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerKills = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerDeaths = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerScore = 0;
};
