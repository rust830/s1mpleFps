#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "s1mpleFpsPvPGameMode.generated.h"

class As1mpleFpsPlayerState;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPvPStatsUpdated, int32, Kills, int32, Deaths, int32, Score);

UCLASS()
class S1MPLEFPS_API As1mpleFpsPvPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	As1mpleFpsPvPGameMode();

	UPROPERTY(BlueprintAssignable)
	FOnPvPStatsUpdated OnStatsUpdated;

	//rules
public:
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float MatchDuration = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 KillLimits = 30;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 OvertimeKillTargets = 1;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	float OvertimeMaxDuration = 120.f;
	void OnKill(APlayerState* KillerPS, APlayerState* VictimPS);
	virtual void BeginPlay()override;
	void OnMatchEnd();

	// ---- 经济系统（蓝图可调） ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	int32 KillRewardBase = 300;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	int32 KillStreakBonus = 100;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
	int32 DeathStreakBonusPerLevel = 50;

	int32 CalculateKillReward(As1mpleFpsPlayerState* Killer) const;

	// 击杀奖励纯函数（供自动化测试，不依赖 Actor/GameMode）
	static int32 ComputeKillReward(int32 Base, int32 StreakBonus, int32 DeathStreakBonusPerLevel, int32 KillStreak, int32 DeathStreak)
	{
		return Base + KillStreak * StreakBonus + DeathStreak * DeathStreakBonusPerLevel;
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WarmUpDuration = 3.0f;


	UFUNCTION()
	void CheckWinnerCondition(As1mpleFpsPlayerState* PS);
};
