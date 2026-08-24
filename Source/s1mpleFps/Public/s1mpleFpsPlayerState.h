// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "s1mpleFpsPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnScoreChanged, int32, Kills, int32, Deaths, int32, Scores);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMoneyChanged, int32, Money);

UENUM(BlueprintType)
enum class ETeam :uint8 {
	None,
	Blue,
	Red
};
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
	/*ID(暂时废弃), 没必要
	UPROPERTY(Replicated, BlueprintReadOnly)
	int32 TeamId = 0;*/
	UPROPERTY(ReplicatedUsing=OnRep_Team,BlueprintReadOnly)
	ETeam Team = ETeam::None;
	UPROPERTY(ReplicatedUsing=OnRep_Ready,BlueprintReadOnly)
	bool bReady = false;
	// 是否为房主（复制，客户端据此显示「开始」按钮）
	UPROPERTY(Replicated, BlueprintReadOnly)
	bool bIsHost = false;

	// 选中的英雄/皮肤索引（复制；大厅选完 → 跨地图携带 → PvP 角色据此换模型）
	UPROPERTY(ReplicatedUsing = OnRep_SelectedHero, BlueprintReadOnly)
	int32 SelectedHeroIndex = 0;

	UPROPERTY(BlueprintAssignable)
	FOnScoreChanged OnScoreChanged;
	UPROPERTY(BlueprintAssignable)
	FOnMoneyChanged OnMoneyChanged;
	UFUNCTION(BlueprintImplementableEvent)
	void OnTeamChanged(ETeam NewTeam);
	UFUNCTION(BlueprintImplementableEvent)
	void OnReadyChanged(bool bIsReady);
	UFUNCTION(BlueprintImplementableEvent)
	void OnHeroChanged(int32 NewHeroIndex);
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

	UFUNCTION()
	void OnRep_Team();

	UFUNCTION()
	void OnRep_Ready();

	UFUNCTION()
	void OnRep_SelectedHero();
};
