// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BuyEquipmentData.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchTimeChanged, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnKillPlay, const FString&, KillerName, const FString&, VictimName, ETeam, KillerTeam);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, const FString&, WinnerName, bool, bWinByKill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSuddenDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOvertimeChanged, float, OvertimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWarmUpTimeChanged, float, WarmUpTimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTeamScoreChanged, int32, BlueKills, int32, RedKills);

// 击杀条颜色规则（类似 LOL 两种显示）
UENUM(BlueprintType)
enum class EKillFeedColorMode : uint8
{
	FixedByTeam,      // 蓝队击杀→蓝、红队击杀→红（按击杀者实际队伍，旁观视角）
	RelativeToViewer  // 自己队击杀→蓝、敌方击杀→红（相对观看者）
};
USTRUCT(BlueprintType)
struct FKillFeedEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	FString KillerName;
	UPROPERTY(BlueprintReadOnly)
	FString VictimName;
	UPROPERTY(BlueprintReadOnly)
	float EntryTime;  // 供 UI 计时删除

};
UCLASS()
class S1MPLEFPS_API As1mpleFpsGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	UPROPERTY(ReplicatedUsing = OnRep_MatchTimeRemaining, BlueprintReadOnly)
	float MatchTimeRemaining = 600.f;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bMatchStarted = false;
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly)
	bool bMatchEnded=false;
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly)
	FString WinnerName;
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly)
	bool bWinByKill;
	UPROPERTY(ReplicatedUsing = OnRep_SuddenDeath, BlueprintReadOnly)
	bool bSuddenDeath = false;
	UPROPERTY(ReplicatedUsing=OnRep_OvertimeChanged,BlueprintReadOnly)
	float OvertimeRemaining = 120.f;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsWarmUp = true;

	UPROPERTY(ReplicatedUsing = OnRep_WarmUpRemaining, EditAnywhere, BlueprintReadWrite)
	float WarmUpTime = 3.0f;

	// === 团队分（复制，供判胜/UI） ===
	UPROPERTY(ReplicatedUsing = OnRep_TeamScore, BlueprintReadOnly)
	int32 BlueTeamKills = 0;
	UPROPERTY(ReplicatedUsing = OnRep_TeamScore, BlueprintReadOnly)
	int32 RedTeamKills = 0;

	// 当前已进行的加时赛轮次（用于「加时次数上限」判断）
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 OvertimeCount = 0;

	// 击杀条颜色规则（复制，客户端据此本地解析颜色）
	UPROPERTY(Replicated, BlueprintReadOnly)
	EKillFeedColorMode KillFeedColorMode = EKillFeedColorMode::FixedByTeam;

	UFUNCTION()
	void AnnounceWinner(const FString& Winner, bool bWin);
	void StartOvertime();
	void OnOvertimeUp();

	// 加时赛起点的团队击杀（服务器本地记录，不复制）
	int32 OvertimeStartBlueKills = 0;
	int32 OvertimeStartRedKills = 0;

	// === 团队辅助 ===
	void AddTeamKill(ETeam Team);
	int32 GetTeamKills(ETeam Team) const;
	int32 GetOvertimeStartTeamKills(ETeam Team) const;
	ETeam GetLeadingTeam() const;                       // 平局返回 ETeam::None
	FString GetTeamName(ETeam Team) const;              // "Blue" / "Red" / "None"
	As1mpleFpsPlayerState* FindStrongestPlayer() const; // 按现有 Kills→Deaths→Scores 优先级
	UPROPERTY(BlueprintAssignable)
	FOnMatchTimeChanged OnMatchTimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnKillPlay OnKillPlay;

	UPROPERTY(BlueprintAssignable)
	FOnMatchEnded OnMatchEnded;

	UPROPERTY(BlueprintAssignable)
	FOnSuddenDeath OnSuddenDeath;

	UPROPERTY(BlueprintAssignable)
	FOnOvertimeChanged OnOvertimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnWarmUpTimeChanged OnWarmUpTimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnTeamScoreChanged OnTeamScoreChanged;

	UPROPERTY(BlueprintAssignable)
	FOnMessageReceived OnMessageReceived;
	
	UFUNCTION(NetMulticast,Unreliable)
	void MulticastKillPlay(const FString& KillerName, const FString& VictimName, ETeam KillerTeam);

	UFUNCTION(NetMulticast,Reliable)
	void MulticastReceivedChatMessage(const FString& Sender, const FString& Message, bool bIsTeam);

	FTimerHandle CountdownHandle;
	UFUNCTION(BlueprintCallable)
	void StartCountdown();

	void TickCountdown();
	void OvertimeCountDown();
	void OnMatchTimeUp();
	void TickWarmUp();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_MatchTimeRemaining();
	UFUNCTION()
	void OnRep_MatchEnded();
	UFUNCTION()
	void OnRep_SuddenDeath();
	UFUNCTION()
	void OnRep_OvertimeChanged();
	UFUNCTION()
	void OnRep_WarmUpRemaining();
	UFUNCTION()
	void OnRep_TeamScore();
	FTimerHandle OvertimeHandle;
	FTimerHandle OvertimeRemainingHandle;
	FTimerHandle WarmUpHandle;
};
