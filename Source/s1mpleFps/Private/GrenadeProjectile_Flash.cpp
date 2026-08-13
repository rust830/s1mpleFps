// Fill out your copyright notice in the Description page of Project Settings.


#include "GrenadeProjectile_Flash.h"
#include "GrenadeData.h"
#include "Engine/OverlapResult.h"
#include "FlashWidget.h"
#include "s1mpleFpsCharacter.h"


void AGrenadeProjectile_Flash::Explode()
{
	MulticastExplosionEffect(GetActorLocation());
	if (HasAuthority()) {
		ApplyFlashEffect();

	}
	Destroy();
}

void AGrenadeProjectile_Flash::ApplyFlashEffect()
{
	if (!GrenadeData)return;
	UWorld* World = GetWorld();
	FVector Center = GetActorLocation();
	float Radius = GrenadeData->FlashRadius;
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity,
		ECC_Pawn, FCollisionShape::MakeSphere(Radius), Params);
	for (const FOverlapResult& Overlap : Overlaps) {
		APawn* Pawn = Cast<APawn>(Overlap.GetActor());
		if (!Pawn || !Pawn->IsPlayerControlled())continue;
		FHitResult TraceHit;
		bool bHit = World->LineTraceSingleByChannel(TraceHit, Center, Pawn->GetActorLocation(),
			ECC_Visibility, Params);
		// 只有射线击中了障碍物且不是受害者本人才跳过（无遮挡时 bHit==false，正常生效）
		if (bHit && TraceHit.GetActor() != Pawn) continue;
		float Distance = FVector::Dist(Center, Pawn->GetActorLocation());
		float DistanceFactor = 1.0f - FMath::Clamp(Distance / Radius, 0.0f, 1.0f);


		FVector ToFlash = (Center - Pawn->GetActorLocation()).GetSafeNormal();
		// 用视线方向(含俯仰)而非 GetActorForwardVector(仅偏航)，
		// 否则闪光弹在地上、玩家低头看它时点积会偏小，导致白闪过淡
		FVector LookDir = Pawn->GetControlRotation().Vector();
		float Dot = FVector::DotProduct(LookDir, ToFlash);
		float AngleFactor = FMath::GetMappedRangeValueClamped(
			FVector2D(-1.0f, 1.0f), FVector2D(0.05f, 1.0f), Dot);


		float TotalScale = DistanceFactor * AngleFactor;
		if (TotalScale < 0.05f) continue;
		// 增强：强度整体放大（FlashIntensityScale 可调），时长给个下限、不再与强度等比缩水
		float Intensity = FMath::Clamp(TotalScale * GrenadeData->FlashIntensityScale, 0.0f, 1.0f);
		float Duration = GrenadeData->FlashDuration * FMath::Clamp(0.5f + 0.5f * TotalScale, 0.0f, 1.0f);

		// Client RPC�����͵������
		APlayerController* PC = Cast<APlayerController>(Pawn->GetController());
		if (PC)
		{
			As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(Pawn);
			if (Char)
			{
				Char->ClientApplyFlash(Intensity, Duration); 
			}
		}
	}
}
