// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class S1MPLEFPS_API ALobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ALobbyGameMode();
	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPC) override;
	virtual void Logout(AController* Exist) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 MinPlayer = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSoftObjectPtr<UWorld> PvPMap;

	// PvP 地图要用的 GameMode（ServerTravel 时显式指定，否则 Listen Server 会把大厅 GameMode 自动带过去）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<AGameModeBase> PvPGameModeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAutoStart = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float AutoStartCountdown = 5.0f;

	void TryStartGame();

	UFUNCTION(BlueprintCallable)
	int32 GetPlayerCount();

	bool CanStartGame();

	bool IsHost(APlayerController* PC) const;
	UFUNCTION(BlueprintCallable)
	bool AreAllPlayersReady()const;
	void CheckStartCondition();
protected:
	TWeakObjectPtr<APlayerController> HostPC;
	FTimerHandle CountdownTimerHandle;
	bool bCountdownStarted = false;


	void StartGame();

	// 蓝图事件：玩家加入大厅
	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerJoinedLobby(APlayerController* NewPlayer);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerLeftLobby(APlayerController* LeavingPlayer);

	UFUNCTION(BlueprintImplementableEvent)
	void OnCountdownStarted(float Duration);
};
