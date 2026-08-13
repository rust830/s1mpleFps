// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdateAlertValue.h"

UBTService_UpdateAlertValue::UBTService_UpdateAlertValue()
{
	NodeName = (TEXT("UpdateAlertValue"));
	Interval = 0.1f;
	bCallTickOnSearchStart = true;
}

void UBTService_UpdateAlertValue::TickNode(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemery, float DeltaTime)
{
	// 已迁移到 AEnemyAIController::Tick() 中统一调用，避免每帧双倍执行
	// Tick() 每帧调用 UpdateAlertValue(DeltaTime)，此处不再重复
}
