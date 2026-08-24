// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "s1mpleFpsLobbyGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLobbyCountdownChanged, float, TimeRemaining);

UCLASS()
class S1MPLEFPS_API As1mpleFpsLobbyGameState : public AGameStateBase
{
	GENERATED_BODY()
public:
	// 开局倒计时剩余（复制，客户端据此显示/隐藏倒计时）
	UPROPERTY(ReplicatedUsing = OnRep_Countdown, BlueprintReadOnly)
	float CountdownRemaining = 0.f;

	UPROPERTY(BlueprintAssignable)
	FOnLobbyCountdownChanged OnLobbyCountdownChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 服务器调用：开始倒计时（每秒 tick 复制剩余时间）
	void ServerStartCountdown(float Duration);
	// 服务器调用：取消倒计时（有人退出 / 取消就绪时）
	void CancelCountdown();

private:
	UFUNCTION()
	void OnRep_Countdown();
	void TickCountdown();
	FTimerHandle CountdownTickHandle;
};
