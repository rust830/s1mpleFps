// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "s1mpleFpsPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnScoreChanged, int32, Kills, int32, Deaths, int32, Scores);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, Money);
UCLASS()
class S1MPLEFPS_API As1mpleFpsPlayerState : public APlayerState
{
	GENERATED_BODY()
public:
	UPROPERTY(ReplicatedUsing = OnRep_Kills, BlueprintReadOnly)
	int32 Kills = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Deaths, BlueprintReadOnly)
	int32 Deaths = 0;
	UPROPERTY(ReplicatedUsing = OnRep_Scores, BlueprintReadOnly)
	int32 Scores = 0;
	UPROPERTY(ReplicatedUsing=OnRep_Money,BlueprintReadOnly)
	int32 Money = 1000;

	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 TeamId = 0;

	UPROPERTY(BlueprintAssignable)
	FOnScoreChanged OnScoreChanged;
	UPROPERTY(BlueprintAssignable)
	FOnMoneyChanged OnMoneyChanged;

	// 连杀/连死（蓝图只读）
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 KillStreak = 0;
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 DeathStreak = 0;

	void IncrementKillStreak();
	void IncrementDeathStreak();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	UFUNCTION()
	void AddKill();
	UFUNCTION()
	void AddDeath();
	UFUNCTION()
	void AddScore(int32 Points);
	UFUNCTION()
	bool TrySpendingMoney(int32 Amount);
	UFUNCTION()
	void AddMoney(int32 Amount);

private:
	UFUNCTION()
	void OnRep_Kills();
	UFUNCTION()
	void OnRep_Deaths();
	UFUNCTION()
	void OnRep_Scores();
	UFUNCTION()
	void OnRep_Money();
};
