// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "BuyEquipmentData.h"
#include "s1mpleFpsGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMatchTimeChanged, float, TimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnKillPlay, const FString&, KillerName, const FString&, VictimName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMatchEnded, const FString&, WinnerName, bool, bWinByKill);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSuddenDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOvertimeChanged, float, OvertimeRemaining);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWarmUpTimeChanged, float, WarmUpTimeRemaining);
USTRUCT(BlueprintType)
struct FKillFeedEntry
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	FString KillerName;
	UPROPERTY(BlueprintReadOnly)
	FString VictimName;
	UPROPERTY(BlueprintReadOnly)
	float EntryTime;  // ���� UI �˼������

};
UCLASS()
class S1MPLEFPS_API As1mpleFpsGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	UPROPERTY(ReplicatedUsing = OnRep_MatchTimeRemaining, BlueprintReadOnly)
	float MatchTimeRemaining = 600.f;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bMatchStarted = false;
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly)
	bool bMatchEnded=false;
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly)
	FString WinnerName;
	UPROPERTY(ReplicatedUsing = OnRep_MatchEnded, BlueprintReadOnly)
	bool bWinByKill;
	UPROPERTY(ReplicatedUsing = OnRep_SuddenDeath, BlueprintReadOnly)
	bool bSuddenDeath = false;
	UPROPERTY(ReplicatedUsing=OnRep_OvertimeChanged,BlueprintReadOnly)
	float OvertimeRemaining = 120.f;
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsWarmUp = true;

	UPROPERTY(ReplicatedUsing = OnRep_WarmUpRemaining, EditAnywhere, BlueprintReadWrite)
	float WarmUpTime = 3.0f;
	
	UFUNCTION()
	void AnnounceWinner(const FString& Winner, bool bWin);
	void StartOvertime();
	void OnOvertimeUp();

	TMap<APlayerState*, int32>OvertimeStartKills;
	UPROPERTY(BlueprintAssignable)
	FOnMatchTimeChanged OnMatchTimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnKillPlay OnKillPlay;

	UPROPERTY(BlueprintAssignable)
	FOnMatchEnded OnMatchEnded;

	UPROPERTY(BlueprintAssignable)
	FOnSuddenDeath OnSuddenDeath;

	UPROPERTY(BlueprintAssignable)
	FOnOvertimeChanged OnOvertimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnWarmUpTimeChanged OnWarmUpTimeChanged;

	UPROPERTY(BlueprintAssignable)
	FOnMessageReceived OnMessageReceived;
	
	UFUNCTION(NetMulticast,Unreliable)
	void MulticastKillPlay(const FString& KillerName, const FString& VictimName);

	UFUNCTION(NetMulticast,Reliable)
	void MulticastReceivedChatMessage(const FString& Sender, const FString& Message, bool bIsTeam);

	FTimerHandle CountdownHandle;
	UFUNCTION(BlueprintCallable)
	void StartCountdown();

	void TickCountdown();
	void OvertimeCountDown();
	void OnMatchTimeUp();
	void TickWarmUp();
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UFUNCTION()
	void OnRep_MatchTimeRemaining();
	UFUNCTION()
	void OnRep_MatchEnded();
	UFUNCTION()
	void OnRep_SuddenDeath();
	UFUNCTION()
	void OnRep_OvertimeChanged();
	UFUNCTION()
	void OnRep_WarmUpRemaining();
	FTimerHandle OvertimeHandle;
	FTimerHandle OvertimeRemainingHandle;
	FTimerHandle WarmUpHandle;
};
