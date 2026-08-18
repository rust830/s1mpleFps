// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "WeaponDataAsset.h"

#if WITH_DEV_AUTOMATION_TESTS

// UWeaponDataAsset::GetMul 对未知骨骼名返回默认 Multiplier（1.0）
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHitZoneUnknownBoneTest, "s1mpleFps.HitZone.UnknownBone", EAutomationTestFlags::EditorContext | EAutomationTestFlags::CommandletContext | EAutomationTestFlags::ProductFilter)
bool FHitZoneUnknownBoneTest::RunTest(const FString& Parameters)
{
	UWeaponDataAsset* Data = NewObject<UWeaponDataAsset>();
	Data->Multiplier = 1.0f;

	FHitZoneEntry Head;
	Head.BoneName = FName(TEXT("head"));
	Head.DamageMul = 4.0f;
	Data->HitZones.Add(Head);

	TestTrue(TEXT("已知骨骼 head → 4x"), FMath::IsNearlyEqual(Data->GetMul(FName(TEXT("head"))), 4.0f));
	TestTrue(TEXT("未知骨骼 → 默认 1x"), FMath::IsNearlyEqual(Data->GetMul(FName(TEXT("some_unknown_bone"))), 1.0f));
	return true;
}

#endif
