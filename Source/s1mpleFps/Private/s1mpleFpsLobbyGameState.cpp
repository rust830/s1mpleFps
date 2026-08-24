// Fill out your copyright notice in the Description page of Project Settings.


#include "s1mpleFpsLobbyGameState.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"

void As1mpleFpsLobbyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(As1mpleFpsLobbyGameState, CountdownRemaining);
}

void As1mpleFpsLobbyGameState::ServerStartCountdown(float Duration)
{
	if (!HasAuthority()) return;
	CountdownRemaining = Duration;
	OnLobbyCountdownChanged.Broadcast(CountdownRemaining); // 服务器本地（listen 房主也更新）
	GetWorld()->GetTimerManager().ClearTimer(CountdownTickHandle);
	GetWorld()->GetTimerManager().SetTimer(CountdownTickHandle, this, &As1mpleFpsLobbyGameState::TickCountdown, 1.0f, true);
}

void As1mpleFpsLobbyGameState::CancelCountdown()
{
	if (!HasAuthority()) return;
	CountdownRemaining = 0.f;
	GetWorld()->GetTimerManager().ClearTimer(CountdownTickHandle);
	OnLobbyCountdownChanged.Broadcast(CountdownRemaining);
}

void As1mpleFpsLobbyGameState::TickCountdown()
{
	if (!HasAuthority()) return;
	CountdownRemaining -= 1.0f;
	if (CountdownRemaining <= 0.f)
	{
		CountdownRemaining = 0.f;
		GetWorld()->GetTimerManager().ClearTimer(CountdownTickHandle);
	}
	OnLobbyCountdownChanged.Broadcast(CountdownRemaining);
}

void As1mpleFpsLobbyGameState::OnRep_Countdown()
{
	OnLobbyCountdownChanged.Broadcast(CountdownRemaining);
}
