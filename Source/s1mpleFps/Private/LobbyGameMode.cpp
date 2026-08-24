// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "s1mpleFpsPlayerState.h"
#include "TimerManager.h"
#include "s1mpleFpsGameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "s1mpleFpsLobbyGameState.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerStateClass = As1mpleFpsPlayerState::StaticClass();
	GameStateClass = As1mpleFpsLobbyGameState::StaticClass();
}

void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
	// 新大厅会话：清空上次比赛遗留的队伍/英雄数据
	if (Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>())
	{
		GI->ClearPlayerTeams();
		GI->ClearPlayerHeroes();
	}
}

void ALobbyGameMode::PostLogin(APlayerController* NewPC)
{
	Super::PostLogin(NewPC);
	if (!HostPC.IsValid()) {
		HostPC = NewPC;
		// 标记房主，复制给客户端用于显示「开始」按钮
		if (As1mpleFpsPlayerState* PS = NewPC->GetPlayerState<As1mpleFpsPlayerState>()) {
			PS->bIsHost = true;
		}
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
	

	return GetNumPlayers() >= MinPlayer&&AreAllPlayersReady();
}

bool ALobbyGameMode::IsHost(APlayerController* PC) const
{	// ListenServer 的主机 GetNetConnection 为 null
	return PC && PC->GetNetConnection() == nullptr;
}

bool ALobbyGameMode::AreAllPlayersReady() const
{
	bool bEveryoneReady = true;
	if (GameState->PlayerArray.Num() <= 0)return false;
	for (APlayerState* PlayerState : GameState->PlayerArray) {
		As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(PlayerState);
		if (PS && !PS->bReady) {
			bEveryoneReady = false;
		}
	}
	return bEveryoneReady;
}

void ALobbyGameMode::CheckStartCondition()
{
	if (!CanStartGame()) {
		// 条件不满足：取消倒计时
		if (bCountdownStarted) {
			bCountdownStarted = false;
			GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
			if (As1mpleFpsLobbyGameState* LobbyGS = GetGameState<As1mpleFpsLobbyGameState>()) {
				LobbyGS->CancelCountdown();
			}
		}
		return;
	}
	// 条件满足：开始倒计时
	if (bAutoStart && !bCountdownStarted) {
		bCountdownStarted = true;
		OnCountdownStarted(AutoStartCountdown);
		if (As1mpleFpsLobbyGameState* LobbyGS = GetGameState<As1mpleFpsLobbyGameState>()) {
			LobbyGS->ServerStartCountdown(AutoStartCountdown);
		}
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
		if (LobbyPS)
		{
			if (LobbyPS->Team == ETeam::None)
			{
				LobbyPS->Team = ETeam::Red;
				LobbyPS->ForceNetUpdate();
			}
			// 把每个玩家的队伍写进 GameInstance，跨 ServerTravel 携带
			if (Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>())
			{
				GI->SetPlayerTeam(LobbyPS->GetUniqueId(), LobbyPS->Team);
			}
		}
	}

	// 设置会话是否可加入；目前游戏中途不支持中途加入，影响不大
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

	// 显式带 GameMode，防止 Listen Server 把当前（大厅）GameMode 自动追加到 URL，导致 PvP 图用了大厅 GameMode
	FString TravelUrl = MapPath + TEXT("?listen");
	if (PvPGameModeClass)
	{
		TravelUrl += TEXT("?game=") + PvPGameModeClass->GetPathName();
	}

	GetWorld()->ServerTravel(TravelUrl);
}


