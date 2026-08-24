// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "ArmorData.h"
#include "WeaponDataAsset.h"
#include "Perception/AISense_Damage.h"
#include "GameFramework/Pawn.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsPvPGameMode.h"
#include "DamageComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamaged,float, Damage, AActor*, Instigator);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class S1MPLEFPS_API UDamageComponent : public UActorComponent
{
	GENERATED_BODY()

public:


		UPROPERTY(BlueprintAssignable)
		FOnDeath OnDeath;
		UPROPERTY(BlueprintAssignable)
		FOnDamaged OnDamaged;
		UDamageComponent();

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float MaxHealth = 100.0f;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float CurrentHealth;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<UArmorData*> EquippedArmors;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UWeaponDataAsset* HitZoneData;

		UPROPERTY(BlueprintReadOnly)
		AActor* LastInstigator = nullptr;

		// 遍历所有护甲，取对该骨骼的最高减伤率
		float GetReductionForBones(FName HitBone) const
		{
			float MaxReduction = 0.0f;
			for (UArmorData* Armor : EquippedArmors)
			{
				if (Armor != nullptr)
				{
					MaxReduction = FMath::Max(MaxReduction, Armor->GetReduction(HitBone));
				}
			}
			return MaxReduction;
		}

		// === 纯函数伤害计算（抽出来供自动化测试，不依赖 World/NetMode） ===
		static float ComputeZoneDamage(float BaseDamage, float HitZoneMul)
		{
			return BaseDamage * HitZoneMul;
		}

		static float ComputeFinalDamage(float ZoneDamage, float Reduction, float Penetration)
		{
			// 有效减伤 = clamp(护甲减伤 - 穿透, 0, 1)，防止 >100% 减伤或负穿透导致负伤害
			const float EffectiveReduction = FMath::Clamp(Reduction - Penetration, 0.0f, 1.0f);
			return ZoneDamage * (1.0f - EffectiveReduction);
		}

		static float ComputeActualDamage(float FinalDamage, float CurrentHealth)
		{
			// 扣血不能超过剩余血量
			return FMath::Min(FinalDamage, CurrentHealth);
		}

		// 核心伤害计算
		float ApplyDamage(FName HitBone, float BaseDamage, float Penetration,
			AActor* Instigator = nullptr, FVector HitLocation = FVector::ZeroVector)
		{
			// 仅服务器（Dedicated 或 Listen）执行伤害。纯客户端跳过。
			if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) {

				return 0.0f;
			}
			// 友军伤害开关：团队模式下默认禁用同队伤害（由 GameMode 的 bFriendlyFireEnabled 控制）
			if (Instigator && GetOwner())
			{
				APawn* VictimPawn = Cast<APawn>(GetOwner());
				APawn* InstigatorPawn = Cast<APawn>(Instigator);
				As1mpleFpsPlayerState* VictimPS = VictimPawn ? Cast<As1mpleFpsPlayerState>(VictimPawn->GetPlayerState()) : nullptr;
				As1mpleFpsPlayerState* InstigatorPS = InstigatorPawn ? Cast<As1mpleFpsPlayerState>(InstigatorPawn->GetPlayerState()) : nullptr;
				if (VictimPS && InstigatorPS && VictimPS->Team != ETeam::None && VictimPS->Team == InstigatorPS->Team)
				{
					if (As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>())
					{
						if (!GM->IsFriendlyFireEnabled()) return 0.0f;
					}
				}
			}
			if (CurrentHealth <= 0.0f) {

				return 0.0f;
			}
			// 1. HitZone 伤害倍率
			float HitX = HitZoneData ? HitZoneData->GetMul(HitBone) : 1.0f;
			// 2. 部位伤害
			float ZoneDamage = ComputeZoneDamage(BaseDamage, HitX);
			// 3. 护甲减伤
			float Reduction = GetReductionForBones(HitBone);
			// 4. 最终伤害（内部含「减伤 - 穿透」的 0~1 clamp）
			float FinalDamage = ComputeFinalDamage(ZoneDamage, Reduction, Penetration);
			// 5. 扣血，不能扣超过剩余血量
			float ActualDamage = ComputeActualDamage(FinalDamage, CurrentHealth);
			CurrentHealth -= ActualDamage;
			
			OnDamaged.Broadcast(ActualDamage, Instigator);
			if (CurrentHealth <= 0.0f) {
				LastInstigator = Instigator;
				OnDeath.Broadcast();
			}
			// Report to AI perception so enemies know they were hit and by whom
			if (Instigator && GetOwner())
			{
				UAISense_Damage::ReportDamageEvent(
					GetWorld(), GetOwner(), Instigator, ActualDamage,
					GetOwner()->GetActorLocation(), HitLocation);
			}
			return ActualDamage;
		}

protected:
		virtual void BeginPlay() override;
		void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction);
};
