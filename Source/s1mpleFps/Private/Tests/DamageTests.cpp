// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "DamageComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

// 部位伤害倍率（头部/身体/四肢）
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageZoneMultiplierTest, "s1mpleFps.Damage.ZoneMultiplier", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FDamageZoneMultiplierTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("头部 x4"), FMath::IsNearlyEqual(UDamageComponent::ComputeZoneDamage(100.0f, 4.0f), 400.0f));
	TestTrue(TEXT("身体 x1"), FMath::IsNearlyEqual(UDamageComponent::ComputeZoneDamage(100.0f, 1.0f), 100.0f));
	TestTrue(TEXT("四肢 x0.75"), FMath::IsNearlyEqual(UDamageComponent::ComputeZoneDamage(100.0f, 0.75f), 75.0f));
	return true;
}

// 护甲减伤 vs 穿透（穿透 >= 减伤 → 减伤归零）
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageArmorReductionTest, "s1mpleFps.Damage.ArmorReduction", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FDamageArmorReductionTest::RunTest(const FString& Parameters)
{
	// 无穿透：减伤 20% → 伤害 x0.8
	TestTrue(TEXT("减伤20%"), FMath::IsNearlyEqual(UDamageComponent::ComputeFinalDamage(100.0f, 0.2f, 0.0f), 80.0f));
	// 穿透 == 减伤：减伤归零
	TestTrue(TEXT("穿透=减伤→归零"), FMath::IsNearlyEqual(UDamageComponent::ComputeFinalDamage(100.0f, 0.2f, 0.2f), 100.0f));
	// 穿透 > 减伤：减伤归零
	TestTrue(TEXT("穿透>减伤→归零"), FMath::IsNearlyEqual(UDamageComponent::ComputeFinalDamage(100.0f, 0.2f, 0.5f), 100.0f));
	// 部分穿透：有效减伤 = 0.5 - 0.2 = 0.3 → x0.7
	TestTrue(TEXT("部分穿透"), FMath::IsNearlyEqual(UDamageComponent::ComputeFinalDamage(100.0f, 0.5f, 0.2f), 70.0f));
	return true;
}

// 最终伤害不能超过剩余血量
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageClampTest, "s1mpleFps.Damage.ClampToHealth", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FDamageClampTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("超出血量→截断"), FMath::IsNearlyEqual(UDamageComponent::ComputeActualDamage(120.0f, 100.0f), 100.0f));
	TestTrue(TEXT("低于血量→原样"), FMath::IsNearlyEqual(UDamageComponent::ComputeActualDamage(50.0f, 100.0f), 50.0f));
	TestTrue(TEXT("正好等于血量"), FMath::IsNearlyEqual(UDamageComponent::ComputeActualDamage(100.0f, 100.0f), 100.0f));
	return true;
}

// 边界：0 伤害 / 减伤 >100% 的 clamp / 负穿透
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDamageBoundaryTest, "s1mpleFps.Damage.Boundaries", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FDamageBoundaryTest::RunTest(const FString& Parameters)
{
	// 0 基础伤害
	TestTrue(TEXT("0 基础伤害"), FMath::IsNearlyEqual(UDamageComponent::ComputeZoneDamage(0.0f, 4.0f), 0.0f));
	// 减伤 >100%：被 clamp 到 1，最终伤害不为负
	TestTrue(TEXT("减伤>100% clamp 到 0"), FMath::IsNearlyEqual(UDamageComponent::ComputeFinalDamage(100.0f, 1.5f, 0.0f), 0.0f));
	// 负穿透：有效减伤被 clamp 到 1，最终伤害不为负
	TestTrue(TEXT("负穿透不产生负伤害"), FMath::IsNearlyEqual(UDamageComponent::ComputeFinalDamage(100.0f, 0.9f, -0.2f), 0.0f));
	return true;
}

#endif
