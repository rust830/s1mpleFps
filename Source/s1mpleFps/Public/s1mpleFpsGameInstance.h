// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOns1FPSFindSessionComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOns1FPSJoinSessionComplete, bool, bWasSuccessful);

UCLASS()
class S1MPLEFPS_API Us1mpleFpsGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void Init() override;

	UFUNCTION(BlueprintCallable)
	void HostGame(const FString& SessionName,const FString& LobbyName, int32 MaxPlayers, bool bIsLAN);
	UFUNCTION(BlueprintCallable)
	void FindSession(int32 MaxSearchResults, bool bIsLAN);
	UFUNCTION(BlueprintCallable)
	void MyJoinSession(int32 SessionIndex);
	UFUNCTION(BlueprintCallable)
	void LeaveSession();

	UFUNCTION(BlueprintCallable)
	void SetSessionJoinable(bool bJoinable);

	UFUNCTION(BlueprintCallable)
	int32 GetSessionCount() const;
	UFUNCTION(BlueprintCallable)
	void GetSessionInfo(int32 Index, FString& OutName, int32& OutPing, int32& OutCurrentPlayers, int32& OutMaxPlayers) const;

	// === 跨地图携带队伍（大厅→PvP） ===
	void SetPlayerTeam(const FUniqueNetIdRepl& PlayerId, ETeam Team);
	ETeam GetPlayerTeam(const FUniqueNetIdRepl& PlayerId) const;
	void ClearPlayerTeams();

	// === 跨地图携带选中英雄（大厅→PvP） ===
	void SetPlayerHero(const FUniqueNetIdRepl& PlayerId, int32 HeroIndex);
	int32 GetPlayerHero(const FUniqueNetIdRepl& PlayerId) const;
	void ClearPlayerHeroes();

	// === 玩家改名（主菜单设置，登录后应用） ===
	UFUNCTION(BlueprintCallable)
	void SetDesiredPlayerName(const FString& Name);
	UFUNCTION(BlueprintCallable)
	FString GetDesiredPlayerName() const;

	// 把地图短名（如 "LobbyMap"）解析成完整包路径（/Game/.../LobbyMap）。
	// PIE 里编辑器会代解析短名，打包后 ServerTravel/OpenLevel 不认短名，必须这里转。
	static FString ResolveMapPath(const FString& MapName);

	UPROPERTY(BlueprintAssignable)
	FOns1FPSFindSessionComplete OnFindSessionComplete;
	UPROPERTY(BlueprintAssignable)
	FOns1FPSJoinSessionComplete OnJoinSessionComplete;

private:
	IOnlineSessionPtr SessionInterface;

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionCompleteInternal(bool bWasSuccessful);
	void OnJoinSessionCompleteInternal(FName InSessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	TSharedPtr<FOnlineSessionSearch> LastSearchResult;
	FString TravelMapName;
	bool bIsHosting = false;
	bool bIsJoining = false;

	// 唯一 ID → 队伍（跨 ServerTravel 携带）
	TMap<FUniqueNetIdRepl, ETeam> PlayerTeams;

	// 唯一 ID → 选中英雄索引（跨 ServerTravel 携带）
	TMap<FUniqueNetIdRepl, int32> PlayerHeroes;

	// 主菜单设置的玩家名
	FString DesiredPlayerName;
};
