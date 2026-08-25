// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "s1mpleFpsPlayerState.h"
#include "ControlArea.generated.h"


class UCapsuleComponent;
class UStaticMeshComponent;
class As1mpleFpsCharacter;
class As1mpleFpsPvPGameMode;
class As1mpleFpsGameState;

UCLASS()
class S1MPLEFPS_API AControlArea : public AActor
{
	GENERATED_BODY()

public:
	AControlArea();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	TObjectPtr<UCapsuleComponent> Zone;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	// 该点单次占点基础分（每点可调）。实际得分 = 基础分 × 时间系数（由 GameMode 按比赛进度缩放）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlArea")
	float AwardScore = 100.f;

	// 当前是否处于激活期（由轮换调度器控制；激活期间才会被抢占得分）
	UPROPERTY(ReplicatedUsing = OnRep_ActiveChanged, BlueprintReadOnly, Category = "ControlArea")
	bool bActive = false;

	// 权威端专用：切换激活状态（关闭时清空圈内记录，避免跨轮次残留）
	void SetActive(bool bNewActive);

	// 圈内人数最多的队伍（平局返回 None）；供蓝图显示/调试用
	UFUNCTION(BlueprintCallable)
	ETeam GetBiggestTeam();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 圈内玩家（服务器本地记录）
	TArray<As1mpleFpsCharacter*> PlayersInZone;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 秒占：圈内只剩单一队伍（排除死亡）→ 立即得分并通知调度器进入下一轮
	void TryCapture();
	ETeam GetSoleTeam() const;
	void AwardCapture(ETeam Team);

	UFUNCTION()
	void OnRep_ActiveChanged();

	// 蓝图视觉反馈（激活高亮 / 被占反馈）
	UFUNCTION(BlueprintImplementableEvent, Category = "ControlArea")
	void OnPointActivated(bool bIsActive);

	UFUNCTION(BlueprintImplementableEvent, Category = "ControlArea")
	void OnPointCaptured(ETeam Team);
};
