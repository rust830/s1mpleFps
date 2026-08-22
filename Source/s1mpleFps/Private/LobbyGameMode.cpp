// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "s1mpleFpsPlayerState.h"
#include "TimerManager.h"
#include "s1mpleFpsGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerStateClass = As1mpleFpsPlayerState::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	//LobbyGameState Need Initializing here;
}

void ALobbyGameMode::PostLogin(APlayerController* NewPC)
{
	Super::PostLogin(NewPC);
	if (!HostPC.IsValid()) {
		HostPC = NewPC;
	}
	OnPlayerJoinedLobby(NewPC);
	CheckStartCondition();
}

void ALobbyGameMode::Logout(AController* Exist)
{
	Super::Logout(Exist);

	if (HostPC == Exist)
	{
		HostPC.Reset();
	}

	if (APlayerController* PC = Cast<APlayerController>(Exist))
	{
		OnPlayerLeftLobby(PC);
	}

	CheckStartCondition();
}

void ALobbyGameMode::TryStartGame()
{
	if (!CanStartGame())return;
	StartGame();
}

int32 ALobbyGameMode::GetPlayerCount()
{
	return GetNumPlayers();
}

bool ALobbyGameMode::CanStartGame()
{
	return GetNumPlayers() >= MinPlayer;
}

bool ALobbyGameMode::IsHost(APlayerController* PC) const
{	//利用ListenServer下Host的GetNetConnection=null
	return PC && PC->GetNetConnection() == nullptr;
}

void ALobbyGameMode::CheckStartCondition()
{
	if (!CanStartGame()) {
		if (bCountdownStarted) {
			bCountdownStarted = false;
			GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		}
	}
	if (bAutoStart && !bCountdownStarted) {
		bCountdownStarted = true;
		OnCountdownStarted(AutoStartCountdown);
		GetWorldTimerManager().SetTimer(
			CountdownTimerHandle,
			this,
			&ALobbyGameMode::StartGame,
			AutoStartCountdown,
			false
		);
	}
}

void ALobbyGameMode::StartGame()
{
	if (!CanStartGame()) return;

	// 给未选阵营的玩家分配默认红队
	for (APlayerState* PS : GameState->PlayerArray)
	{
		As1mpleFpsPlayerState* LobbyPS = Cast<As1mpleFpsPlayerState>(PS);
		if (LobbyPS && LobbyPS->Team == ETeam::None)
		{
			LobbyPS->Team = ETeam::Red;
			LobbyPS->ForceNetUpdate();
		}
	}

	// 设置是否能加入,目前的游戏性质是支持加入的,影响不大
	if (Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>())
	{
		GI->SetSessionJoinable(true);
	}
	FString MapPath = PvPMap.GetLongPackageName(); 
	if (MapPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("PvPMap is not set!"));
		return;
	}

	GetWorld()->ServerTravel(MapPath + TEXT("?listen"));
}


