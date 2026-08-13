// Fill out your copyright notice in the Description page of Project Settings.


#include "BTService_UpdateBlackboard.h"

UBTService_UpdateBlackboard::UBTService_UpdateBlackboard()
{
	NodeName = (TEXT("UpdateBlackboard"));
	Interval = 0.2f;
	bCallTickOnSearchStart = true;
}

void UBTService_UpdateBlackboard::TickNode(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemery, float DeltaTime)
{
	// 已迁移到 AEnemyAIController::Tick() 中统一调用，避免每帧双倍执行
	// Tick() 每帧调用 UpdateBlackboardState()，此处不再重复
}
