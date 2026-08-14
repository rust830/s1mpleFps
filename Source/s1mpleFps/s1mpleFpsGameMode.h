// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "s1mpleFpsGameMode.generated.h"

class AEnemyCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStatsUpdated, int32, Kills, int32, Deaths, int32, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMissionResult, bool, bWin);

UCLASS(minimalapi)
class As1mpleFpsGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	As1mpleFpsGameMode();

	UPROPERTY(BlueprintAssignable)
	FOnStatsUpdated OnStatsUpdated;

	// 任务结算事件：bWin=true 获胜 / false 失败（蓝图/HUD 绑定显示结算界面）
	UPROPERTY(BlueprintAssignable)
	FOnMissionResult OnMissionResult;

	void OnKill(AActor* Killer, AActor* Victim);

	void ScheduleEnemyRespawn(TSubclassOf<AEnemyCharacter> EnemyClass, FVector SpawnLoc, FRotator SpawnRot, float Delay);

	// ---- 胜负条件（蓝图可调） ----
	// 击杀 N 个敌人获胜
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	int32 EnemyKillTarget = 30;
	// 玩家死亡即失败（关闭则为无尽模式，仅击杀目标获胜）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mission")
	bool bFailOnPlayerDeath = true;

	// 任务状态（蓝图只读）
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bMissionCompleted = false;
	UPROPERTY(BlueprintReadOnly, Category = "Mission")
	bool bMissionFailed = false;

	// 玩家死亡回调：返回 false 表示任务失败、不再复活
	bool OnPlayerDeath();
	// 击杀后判定是否达到获胜目标
	void CheckWinCondition();

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerKills = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerDeaths = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 PlayerScore = 0;
};
