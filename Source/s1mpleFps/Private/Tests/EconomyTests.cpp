// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsPvPGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS

// TrySpendingMoney：够钱 / 不够 / 正好花完
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomySpendTest, "s1mpleFps.Economy.Spending", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FEconomySpendTest::RunTest(const FString& Parameters)
{
	As1mpleFpsPlayerState* PS = NewObject<As1mpleFpsPlayerState>();
	TestEqual(TEXT("初始 1000"), PS->Money, 1000);

	TestTrue(TEXT("够钱"), PS->TrySpendingMoney(300));
	TestEqual(TEXT("剩 700"), PS->Money, 700);

	TestFalse(TEXT("不够"), PS->TrySpendingMoney(1000));
	TestEqual(TEXT("仍 700"), PS->Money, 700);

	TestTrue(TEXT("正好花完"), PS->TrySpendingMoney(700));
	TestEqual(TEXT("剩 0"), PS->Money, 0);
	return true;
}

// AddMoney / AddScore / AddKill / AddDeath
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyStatsTest, "s1mpleFps.Economy.Stats", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FEconomyStatsTest::RunTest(const FString& Parameters)
{
	As1mpleFpsPlayerState* PS = NewObject<As1mpleFpsPlayerState>();

	PS->AddMoney(500);
	TestEqual(TEXT("Money 1500"), PS->Money, 1500);

	PS->AddScore(50);
	TestEqual(TEXT("Scores 50"), PS->Scores, 50);

	PS->AddDeath();
	TestEqual(TEXT("Deaths 1"), PS->Deaths, 1);

	PS->AddKill();
	TestEqual(TEXT("Kills 1"), PS->Kills, 1);
	TestEqual(TEXT("Scores 150"), PS->Scores, 150); // AddKill 固定 +100
	return true;
}

// 击杀奖励 + 连死补偿（纯函数）
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEconomyKillRewardTest, "s1mpleFps.Economy.KillReward", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FEconomyKillRewardTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("基础奖励"), As1mpleFpsPvPGameMode::ComputeKillReward(300, 100, 50, 0, 0), 300);
	TestEqual(TEXT("连杀加成"), As1mpleFpsPvPGameMode::ComputeKillReward(300, 100, 50, 3, 0), 600);
	TestEqual(TEXT("连死补偿"), As1mpleFpsPvPGameMode::ComputeKillReward(300, 100, 50, 0, 2), 400);
	TestEqual(TEXT("连杀+连死"), As1mpleFpsPvPGameMode::ComputeKillReward(300, 100, 50, 2, 3), 650);
	return true;
}

#endif
