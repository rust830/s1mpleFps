// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/DelegateCombinations.h"
#include "ArmorData.h"
#include "WeaponDataAsset.h"
#include "Perception/AISense_Damage.h"
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

		// 核心伤害计算
		float ApplyDamage(FName HitBone, float BaseDamage, float Penetration,
			AActor* Instigator = nullptr, FVector HitLocation = FVector::ZeroVector)
		{
			// 仅服务器（Dedicated 或 Listen）执行伤害。纯客户端跳过。
			if (GetWorld() && GetWorld()->GetNetMode() == NM_Client) {
				
				return 0.0f;
			}
			if (CurrentHealth <= 0.0f) {
				
				return 0.0f;
			}
			// 1. HitZone 伤害倍率
			float HitX = HitZoneData ? HitZoneData->GetMul(HitBone) : 1.0f;
			// 2. 部位伤害
			float ZoneDamage = BaseDamage * HitX;
			// 3. 护甲减伤
			float Reduction = GetReductionForBones(HitBone);
			// 4. 有效减伤 = 护甲减伤 - 穿透
			float EffectiveReduction = FMath::Max(0.0f, Reduction - Penetration);
			// 5. 最终伤害 = 部位伤害 × (1 - 有效减伤)
			float FinalDamage = ZoneDamage * (1.0f - EffectiveReduction);
			// 6. 扣血，不能扣超过剩余血量
			float ActualDamage = (CurrentHealth - FinalDamage > 0) ? FinalDamage : CurrentHealth;
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
