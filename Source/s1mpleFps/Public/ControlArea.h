// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "s1mpleFpsPlayerState.h"
#include "ControlArea.generated.h"


class UCapsuleComponent;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USoundBase;
class As1mpleFpsCharacter;
class As1mpleFpsPvPGameMode;
class As1mpleFpsGameState;

UCLASS()
class S1MPLEFPS_API AControlArea : public AActor
{
	GENERATED_BODY()

public:
	AControlArea();
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	TObjectPtr<UCapsuleComponent> Zone;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	// 该点单次占点基础分（每点可调）。实际得分 = 基础分 × 时间系数（由 GameMode 按比赛进度缩放）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	float AwardScore = 100.f;

	// 得分时间：占领方持续站满这段时间后得分（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	float ScoreTime = 5.f;

	// 占领时间：更换占领方（中性点被争夺 / 敌方抢点）需持续站满的时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	float ControlTime = 3.f;

	// 激活音效（该点轮换到激活时播放）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea|Sound")
	USoundBase* ActivationSound = nullptr;
	// 被占领得分音效（占点成功得分时播放）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea|Sound")
	USoundBase* CapturedSound = nullptr;

	// 当前是否处于激活期（由轮换调度器控制；激活期间才会被抢占得分）
	UPROPERTY(ReplicatedUsing = OnRep_ActiveChanged, BlueprintReadOnly, Category = "ControlArea")
	bool bActive = false;

	// 当前占领方（None=中性/未占领）。由服务器 Tick 计算，复制给客户端供 UI 显示
	UPROPERTY(ReplicatedUsing = OnRep_CaptureState, BlueprintReadOnly, Category = "ControlArea")
	ETeam ControllingTeam = ETeam::None;

	// 得分进度 0~1（占领方走得分时间）；复制给客户端画进度条
	UPROPERTY(ReplicatedUsing = OnRep_CaptureState, BlueprintReadOnly, Category = "ControlArea")
	float ScoreProgress = 0.f;

	// 占领进度 0~1（更换占领方走占领时间）；复制给客户端画进度条
	UPROPERTY(ReplicatedUsing = OnRep_CaptureState, BlueprintReadOnly, Category = "ControlArea")
	float ControlProgress = 0.f;

	// 正在抢点的那一方（走占领时间中；None=当前没有换边）。复制给客户端画进度条方向/颜色
	UPROPERTY(ReplicatedUsing = OnRep_CaptureState, BlueprintReadOnly, Category = "ControlArea")
	ETeam ControlAttemptTeam = ETeam::None;

	// 权威端专用：切换激活状态（关闭时清空圈内记录，避免跨轮次残留）
	void SetActive(bool bNewActive);

	// 圈内人数最多的队伍（平局返回 None）；供蓝图显示/调试用
	UFUNCTION(BlueprintCallable)
	ETeam GetBiggestTeam();

	// 激活/被占音效广播（服务器 → 所有端），在点的位置做 3D 定位音
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayActivationSound();
	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayCapturedSound();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 动态材质实例：根据 bActive 更新 EnableFlicker 参数（激活闪烁）
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterialInstance;

	// 需要更新参数的材质槽索引（通常 0）
	UPROPERTY(EditAnywhere, Category = "ControlArea")
	int32 MaterialSlotIndex = 0;

	void CreateDynamicMaterial();
	void UpdateFlickerMaterial();

	// 圈内玩家（服务器本地记录）
	TArray<As1mpleFpsCharacter*> PlayersInZone;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 占点状态机（每帧推进，仅服务器）
	void TickControl(float DeltaSeconds);
	// 圈内人数最多的队伍（死亡者不计；平局/空圈返回 None）
	ETeam GetMajorityTeam() const;
	// 是否争夺：圈内同时存在两支存活队伍
	bool IsContested() const;
	// 得分 + 通知调度器进入下一轮（占领/得分完成后调用）
	void AwardCapture(ETeam Team);

	UFUNCTION()
	void OnRep_ActiveChanged();
	UFUNCTION()
	void OnRep_CaptureState();

	// 蓝图视觉反馈（激活高亮 / 被占反馈）
	UFUNCTION(BlueprintImplementableEvent, Category = "ControlArea")
	void OnPointActivated(bool bIsActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "ControlArea")
	void OnPointCaptured(ETeam Team);

	// 蓝图进度反馈（驱动占点进度条 UI）：占领方 / 正在抢点方 / 得分进度 / 占领进度
	UFUNCTION(BlueprintImplementableEvent, Category = "ControlArea")
	void OnCaptureProgressChanged(ETeam Controlling, ETeam Capturing, float ScorePct, float ControlPct);
};
