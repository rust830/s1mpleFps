// Fill out your copyright notice in the Description page of Project Settings.


#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsPvPGameMode.h"
#include "Net/UnrealNetwork.h"

void As1mpleFpsGameState::MulticastKillPlay_Implementation(const FString& KillerName, const FString& VictimName)
{
	OnKillPlay.Broadcast(KillerName, VictimName);
}

void As1mpleFpsGameState::MulticastReceivedChatMessage_Implementation(const FString& Sender, const FString& Message, bool bIsTeam)
{
	OnMessageReceived.Broadcast(Sender, Message, bIsTeam);
}

void As1mpleFpsGameState::StartCountdown()
{
	if (HasAuthority()) {
		
		bIsWarmUp = true;
		if (As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>()) {
			WarmUpTime = GM->WarmUpDuration;
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
	if (bMatchEnded)return;
	int highestKill = -1;
	TArray<FString> TopPlayers;
	for (APlayerState* PS : PlayerArray) {
		As1mpleFpsPlayerState* PvPPS = Cast<As1mpleFpsPlayerState>(PS);
		if (PvPPS && PvPPS->Kills > highestKill) {
			highestKill = PvPPS->Kills;
			TopPlayers.Empty();
			TopPlayers.Add( PvPPS->GetPlayerName());
		}
		else if (PvPPS && PvPPS->Kills == highestKill) {
			TopPlayers.Add(PvPPS->GetPlayerName());
		}
	}
	if (TopPlayers.Num() == 1) {
		AnnounceWinner(TopPlayers[0], false);
		return;
	}
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
	if (!HasAuthority())return;
	OvertimeStartKills.Empty();
	for (APlayerState* PS : PlayerArray) {
		As1mpleFpsPlayerState* Player = Cast<As1mpleFpsPlayerState>(PS);
		if (Player) {
			OvertimeStartKills.Add(Player, Player->Kills);
				}
	}
	As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>();
	float MaxTime = GM ? GM->OvertimeMaxDuration : 120.f;
	GetWorld()->GetTimerManager().SetTimer(OvertimeHandle, this, &As1mpleFpsGameState::OnOvertimeUp, MaxTime, false);
	OvertimeRemaining = MaxTime;
	OnOvertimeChanged.Broadcast(OvertimeRemaining);
	GetWorld()->GetTimerManager().SetTimer(OvertimeRemainingHandle, this, &As1mpleFpsGameState::OvertimeCountDown, 1.0f, true);
}

void As1mpleFpsGameState::OnOvertimeUp()
{
	if (bMatchEnded)return;
	int32 BestKills = -1;
	int32 BestDeaths = 999;
	int32 BestScore = -1;
	FString Winner;
	for (APlayerState* PS : PlayerArray) {
		As1mpleFpsPlayerState* Player = Cast<As1mpleFpsPlayerState>(PS);
		if (!Player)continue;

		bool bBetter = false;
		if (Player->Kills > BestKills) { bBetter = true; }
		else if (Player->Kills == BestKills && Player->Deaths < BestDeaths) { bBetter = true; }
		else if (Player->Kills == BestKills && Player->Deaths == BestDeaths && Player->Scores > BestScore) { bBetter = true; }

		if (bBetter) {
			BestKills = Player->Kills;
			BestDeaths = Player->Deaths;
			BestScore = Player->Scores;
			Winner = Player->GetPlayerName();
		}
	}
	AnnounceWinner(Winner, false);
}
