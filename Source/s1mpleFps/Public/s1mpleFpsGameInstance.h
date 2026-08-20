// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
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
	void JoinSession(int32 SessionIndex);
	UFUNCTION(BlueprintCallable)
	void LeaveSession();

	UFUNCTION(BlueprintCallable)
	void SetSessionJoinable(bool bJoinable);

	UFUNCTION(BlueprintCallable)
	int32 GetSessionCount() const;
	UFUNCTION(BlueprintCallable)
	void GetSessionInfo(int32 Index, FString& OutName, int32& OutPing, int32& OutCurrentPlayers, int32& OutMaxPlayers) const;

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
};
