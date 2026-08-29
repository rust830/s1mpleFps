// Fill out your copyright notice in the Description page of Project Settings.


#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsPvPGameMode.h"
#include "Net/UnrealNetwork.h"

void As1mpleFpsGameState::MulticastKillPlay_Implementation(const FString& KillerName, const FString& VictimName, ETeam KillerTeam)
{
	OnKillPlay.Broadcast(KillerName, VictimName, KillerTeam);
}

void As1mpleFpsGameState::MulticastReceivedChatMessage_Implementation(const FString& Sender, const FString& Message, bool bIsTeam)
{
	OnMessageReceived.Broadcast(Sender, Message, bIsTeam);
}

void As1mpleFpsGameState::MulticastControlPointScored_Implementation(ETeam Team, int32 Score)
{
	OnControlPointScored.Broadcast(Team, Score);
}

void As1mpleFpsGameState::StartCountdown()
{
	if (HasAuthority()) {
		
		bIsWarmUp = true;
		if (As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>()) {
			WarmUpTime = GM->WarmUpDuration;
			KillFeedColorMode = GM->KillFeedColorMode;
		}
		OnWarmUpTimeChanged.Broadcast(WarmUpTime);
		GetWorld()->GetTimerManager().SetTimer(WarmUpHandle, this, &As1mpleFpsGameState::TickWarmUp, 1.0f, true);

	}
}

void As1mpleFpsGameState::TickCountdown()
{
	if (!HasAuthority()) return;
	if (bMatchEnded) {
		GetWorld()->GetTimerManager().ClearTimer(CountdownHandle);
		return;
	}
	if (MatchTimeRemaining <= 0) return;
	MatchTimeRemaining -= 1;
	
	OnMatchTimeChanged.Broadcast(MatchTimeRemaining);
	if (MatchTimeRemaining <= 0)
	{
		MatchTimeRemaining = 0;
		GetWorldTimerManager().ClearTimer(CountdownHandle);
		OnMatchTimeUp();
	}
}

void As1mpleFpsGameState::OvertimeCountDown()
{
	if (!HasAuthority() || bMatchEnded)return;
	OvertimeRemaining -= 1;
	OnOvertimeChanged.Broadcast(OvertimeRemaining);
	if (OvertimeRemaining <= 0) {
		OvertimeRemaining = 0;
		GetWorld()->GetTimerManager().ClearTimer(OvertimeRemainingHandle);
	}
}

void As1mpleFpsGameState::OnMatchTimeUp()
{
	if (bMatchEnded) return;
	// 时间到：团队积分多者获胜（同分看击杀）
	ETeam Leading = GetLeadingTeam();
	if (Leading != ETeam::None) {
		AnnounceWinner(GetTeamName(Leading), false);
		return;
	}
	// 平局 → 加时
	bSuddenDeath = true;
	OnSuddenDeath.Broadcast();
	StartOvertime();
}

void As1mpleFpsGameState::TickWarmUp()
{
	if (!HasAuthority())return;
	WarmUpTime -= 1;
	
	OnWarmUpTimeChanged.Broadcast(WarmUpTime);
	if (WarmUpTime <= 0.f) {
		WarmUpTime = 0;
		bIsWarmUp = false;
		GetWorldTimerManager().ClearTimer(WarmUpHandle);

		bMatchStarted = true;

		// 比赛正式开始：启动占点轮换（热身期间不激活任何点）
		if (As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>())
		{
			GM->BeginControlRotation();
		}

		GetWorldTimerManager().SetTimer(CountdownHandle, this, &As1mpleFpsGameState::TickCountdown, 1.0f, true);
	}
}

void As1mpleFpsGameState::BeginPlay()
{
	Super::BeginPlay();

}

void As1mpleFpsGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(As1mpleFpsGameState, MatchTimeRemaining);
	DOREPLIFETIME(As1mpleFpsGameState, bMatchStarted);
	DOREPLIFETIME(As1mpleFpsGameState, WinnerName);
	DOREPLIFETIME(As1mpleFpsGameState, bMatchEnded);
	DOREPLIFETIME(As1mpleFpsGameState,bWinByKill);
	DOREPLIFETIME(As1mpleFpsGameState, bSuddenDeath);
	DOREPLIFETIME(As1mpleFpsGameState, OvertimeRemaining);
	DOREPLIFETIME(As1mpleFpsGameState, WarmUpTime);
	DOREPLIFETIME(As1mpleFpsGameState, bIsWarmUp);
	DOREPLIFETIME(As1mpleFpsGameState, BlueTeamKills);
	DOREPLIFETIME(As1mpleFpsGameState, RedTeamKills);
	DOREPLIFETIME(As1mpleFpsGameState, BlueTeamScore);
	DOREPLIFETIME(As1mpleFpsGameState, RedTeamScore);
	DOREPLIFETIME(As1mpleFpsGameState, OvertimeCount);
	DOREPLIFETIME(As1mpleFpsGameState, KillFeedColorMode);
}

void As1mpleFpsGameState::OnRep_MatchTimeRemaining()
{
	OnMatchTimeChanged.Broadcast(MatchTimeRemaining);
}

void As1mpleFpsGameState::OnRep_MatchEnded()
{
	OnMatchEnded.Broadcast(WinnerName, bWinByKill);
}

void As1mpleFpsGameState::OnRep_SuddenDeath()
{
	OnSuddenDeath.Broadcast();
}

void As1mpleFpsGameState::OnRep_OvertimeChanged()
{
	OnOvertimeChanged.Broadcast(OvertimeRemaining);
}

void As1mpleFpsGameState::OnRep_WarmUpRemaining()
{
	OnWarmUpTimeChanged.Broadcast(WarmUpTime);
}

void As1mpleFpsGameState::OnRep_TeamScore()
{
	OnTeamScoreChanged.Broadcast(BlueTeamScore, RedTeamScore, BlueTeamKills, RedTeamKills);
}

// === 团队辅助 ===
void As1mpleFpsGameState::AddTeamKill(ETeam Team)
{
	if (Team == ETeam::Blue) {
		BlueTeamKills++;
	}
	else if (Team == ETeam::Red) {
		RedTeamKills++;
	}
	OnTeamScoreChanged.Broadcast(BlueTeamScore, RedTeamScore, BlueTeamKills, RedTeamKills);
}

void As1mpleFpsGameState::AddTeamScore(ETeam Team, int32 Points)
{
	if (Team == ETeam::Blue) {
		BlueTeamScore += Points;
	}
	else if (Team == ETeam::Red) {
		RedTeamScore += Points;
	}
	OnTeamScoreChanged.Broadcast(BlueTeamScore, RedTeamScore, BlueTeamKills, RedTeamKills);
}

int32 As1mpleFpsGameState::GetTeamScore(ETeam Team) const
{
	if (Team == ETeam::Blue) return BlueTeamScore;
	if (Team == ETeam::Red) return RedTeamScore;
	return 0;
}

int32 As1mpleFpsGameState::GetTeamKills(ETeam Team) const
{
	if (Team == ETeam::Blue) return BlueTeamKills;
	if (Team == ETeam::Red) return RedTeamKills;
	return 0;
}

int32 As1mpleFpsGameState::GetOvertimeStartTeamKills(ETeam Team) const
{
	if (Team == ETeam::Blue) return OvertimeStartBlueKills;
	if (Team == ETeam::Red) return OvertimeStartRedKills;
	return 0;
}

ETeam As1mpleFpsGameState::GetLeadingTeam() const
{
	// 主条件：团队积分（占点得分）多者领先
	if (BlueTeamScore != RedTeamScore)
		return BlueTeamScore > RedTeamScore ? ETeam::Blue : ETeam::Red;
	// 同分：击杀多者领先
	if (BlueTeamKills != RedTeamKills)
		return BlueTeamKills > RedTeamKills ? ETeam::Blue : ETeam::Red;
	return ETeam::None;
}

FString As1mpleFpsGameState::GetTeamName(ETeam Team) const
{
	switch (Team) {
	case ETeam::Blue: return TEXT("Blue");
	case ETeam::Red:  return TEXT("Red");
	default:          return TEXT("None");
	}
}

As1mpleFpsPlayerState* As1mpleFpsGameState::FindStrongestPlayer() const
{
	As1mpleFpsPlayerState* Best = nullptr;
	int32 BestKills = -1;
	int32 BestDeaths = 999;
	int32 BestScore = -1;
	for (APlayerState* PS : PlayerArray) {
		As1mpleFpsPlayerState* Player = Cast<As1mpleFpsPlayerState>(PS);
		if (!Player) continue;

		bool bBetter = false;
		if (Player->Kills > BestKills) { bBetter = true; }
		else if (Player->Kills == BestKills && Player->Deaths < BestDeaths) { bBetter = true; }
		else if (Player->Kills == BestKills && Player->Deaths == BestDeaths && Player->Scores > BestScore) { bBetter = true; }

		if (bBetter) {
			Best = Player;
			BestKills = Player->Kills;
			BestDeaths = Player->Deaths;
			BestScore = Player->Scores;
		}
	}
	return Best;
}


void As1mpleFpsGameState::AnnounceWinner(const FString& Winner, bool bWin)
{
	if (!HasAuthority() || bMatchEnded)return;
	bMatchEnded = true;
	WinnerName = Winner;
	bWinByKill = bWin;
	GetWorld()->GetTimerManager().ClearTimer(CountdownHandle);
	GetWorld()->GetTimerManager().ClearTimer(OvertimeHandle);
	GetWorld()->GetTimerManager().ClearTimer(OvertimeRemainingHandle);
	OnMatchEnded.Broadcast(WinnerName, bWinByKill);

}

void As1mpleFpsGameState::StartOvertime()
{
	if (!HasAuthority()) return;
	OvertimeCount++;
	OvertimeStartBlueKills = BlueTeamKills;
	OvertimeStartRedKills = RedTeamKills;

	As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>();
	float MaxTime = GM ? GM->OvertimeMaxDuration : 120.f;

	// 可重复加时：先清掉上一轮残留计时器再开新轮
	GetWorld()->GetTimerManager().ClearTimer(OvertimeHandle);
	GetWorld()->GetTimerManager().ClearTimer(OvertimeRemainingHandle);

	GetWorld()->GetTimerManager().SetTimer(OvertimeHandle, this, &As1mpleFpsGameState::OnOvertimeUp, MaxTime, false);
	OvertimeRemaining = MaxTime;
	OnOvertimeChanged.Broadcast(OvertimeRemaining);
	GetWorld()->GetTimerManager().SetTimer(OvertimeRemainingHandle, this, &As1mpleFpsGameState::OvertimeCountDown, 1.0f, true);
}

void As1mpleFpsGameState::OnOvertimeUp()
{
	if (bMatchEnded) return;
	ETeam Leading = GetLeadingTeam();
	if (Leading != ETeam::None) {
		AnnounceWinner(GetTeamName(Leading), false);
		return;
	}

	// 平局：未达加时上限则再开一轮
	As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>();
	int32 MaxRounds = GM ? GM->MaxOvertimeRounds : 1;
	if (OvertimeCount < MaxRounds) {
		StartOvertime();
		return;
	}

	// 加时次数达到上限：比最强个人（沿用现有 Kills→Deaths→Scores 优先级）
	As1mpleFpsPlayerState* Strongest = FindStrongestPlayer();
	if (Strongest) {
		AnnounceWinner(GetTeamName(Strongest->Team), false);
	}
}
