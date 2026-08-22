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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bAutoStart = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float AutoStartCountdown = 5.0f;

	void TryStartGame();

	UFUNCTION(BlueprintCallable)
	int32 GetPlayerCount();

	bool CanStartGame();
	bool IsHost(APlayerController* PC) const;
protected:
	TWeakObjectPtr<APlayerController> HostPC;
	FTimerHandle CountdownTimerHandle;
	bool bCountdownStarted = false;

	void CheckStartCondition();
	void StartGame();

	// 蓝图事件（可选）
	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerJoinedLobby(APlayerController* NewPlayer);

	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerLeftLobby(APlayerController* LeavingPlayer);

	UFUNCTION(BlueprintImplementableEvent)
	void OnCountdownStarted(float Duration);
};
