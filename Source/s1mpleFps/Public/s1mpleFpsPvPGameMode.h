#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPvPGameMode.generated.h"

class As1mpleFpsPlayerState;
class USoundBase;

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
	// 加时赛最多打几轮（超过则比最强个人定胜负）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rules")
	int32 MaxOvertimeRounds = 3;
	void OnKill(APlayerState* KillerPS, APlayerState* VictimPS);
	virtual void BeginPlay()override;
	virtual void PostLogin(APlayerController* NewPC) override;
	// 无缝跳图不会走 PostLogin，用这个钩子补上大厅选好的队伍 + 英雄恢复
	virtual void HandleSeamlessTravelPlayer(AController*& C) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;
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

	// ---- 团队规则 ----
	// 是否允许友军伤害（true=可以误伤队友，false=同队伤害无效）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	bool bFriendlyFireEnabled = false;
	bool IsFriendlyFireEnabled() const { return bFriendlyFireEnabled; }

	// ---- 击杀/连杀音效（服务器 → 击杀者本人播放） ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* KillSound = nullptr;
	// 多杀音效：索引 0=双杀, 1=三杀, 2=四杀...（按 KillStreak-2 取，超出取最后一个）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TArray<USoundBase*> MultiKillSounds;

	// 击杀条颜色规则（两种模式，见 EKillFeedColorMode）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
	EKillFeedColorMode KillFeedColorMode = EKillFeedColorMode::FixedByTeam;

	UFUNCTION()
	void CheckWinnerCondition(As1mpleFpsPlayerState* PS);

private:
	// 从 GameInstance 恢复大厅选好的队伍 + 英雄（普通登录走 PostLogin，无缝跳图走 HandleSeamlessTravelPlayer）
	void RestoreCarriedPlayer(AController* NewPC);
};
