// Fill out your copyright notice in the Description page of Project Settings.


#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsPvPGameMode.h"
#include "s1mpleFpsCharacter.h"
#include "Net/UnrealNetwork.h"

void As1mpleFpsPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(As1mpleFpsPlayerState, Kills);
	DOREPLIFETIME(As1mpleFpsPlayerState, Deaths);
	DOREPLIFETIME(As1mpleFpsPlayerState, Scores);
	DOREPLIFETIME(As1mpleFpsPlayerState, Money);
	DOREPLIFETIME_CONDITION(As1mpleFpsPlayerState, KillStreak, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(As1mpleFpsPlayerState, DeathStreak, COND_OwnerOnly);
	//DOREPLIFETIME(As1mpleFpsPlayerState, TeamId);
	DOREPLIFETIME(As1mpleFpsPlayerState, Team);
	DOREPLIFETIME(As1mpleFpsPlayerState, bReady);
	DOREPLIFETIME(As1mpleFpsPlayerState, bIsHost);
	DOREPLIFETIME(As1mpleFpsPlayerState, SelectedHeroIndex);
}

void As1mpleFpsPlayerState::OnRep_Kills()
{
	OnScoreChanged.Broadcast(Kills, Deaths, Scores);
}

void As1mpleFpsPlayerState::OnRep_Deaths()
{
	OnScoreChanged.Broadcast(Kills, Deaths, Scores);
}

void As1mpleFpsPlayerState::OnRep_Scores()
{
	OnScoreChanged.Broadcast(Kills, Deaths, Scores);
}

void As1mpleFpsPlayerState::OnRep_Money()
{
	OnMoneyChanged.Broadcast(Money);
}

void As1mpleFpsPlayerState::OnRep_Team()
{
	OnTeamChanged(Team);
}

void As1mpleFpsPlayerState::OnRep_Ready()
{
	OnReadyChanged(bReady);
}

void As1mpleFpsPlayerState::OnRep_SelectedHero()
{
	// 只通知 UI（选人高亮等）；模型换肤已改由 Character 自身的复制字段驱动（OnRep_HeroVisual）
	OnHeroChanged(SelectedHeroIndex);
}

void As1mpleFpsPlayerState::AddKill()
{
	Kills++;
	Scores += 100;
	OnScoreChanged.Broadcast(Kills, Deaths, Scores);

	// 权威端：击杀写入后立即判定胜负（事件驱动，避免 GameMode 每帧轮询）
	if (GetWorld())
	{
		if (As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>())
		{
			GM->CheckWinnerCondition(this);
		}
	}
}

void As1mpleFpsPlayerState::AddDeath()
{
	Deaths++;
	OnScoreChanged.Broadcast(Kills, Deaths, Scores);
}

void As1mpleFpsPlayerState::AddScore(int32 Points)
{
	Scores += Points;
	OnScoreChanged.Broadcast(Kills, Deaths, Scores);
}

bool As1mpleFpsPlayerState::TrySpendingMoney(int32 Amount)
{
	if (Money >= Amount) {
		Money -= Amount;
		OnMoneyChanged.Broadcast(Money);
		return true;
	}
	return false;
}

void As1mpleFpsPlayerState::AddMoney(int32 Amount)
{
	Money += Amount;
	OnMoneyChanged.Broadcast(Money);
}

void As1mpleFpsPlayerState::IncrementKillStreak()
{
	KillStreak++;
	DeathStreak = 0;
}

void As1mpleFpsPlayerState::IncrementDeathStreak()
{
	DeathStreak++;
	KillStreak = 0;
}
