// Fill out your copyright notice in the Description page of Project Settings.


#include "ControlArea.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "s1mpleFpsCharacter.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPvPGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Net/UnrealNetwork.h"

AControlArea::AControlArea()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	Zone = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Zone"));
	Zone->SetCapsuleSize(200.f, 100.f);
	Zone->SetCollisionProfileName(TEXT("ZoneCollision"));
	RootComponent = Zone;

	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(RootComponent);
}

void AControlArea::BeginPlay()
{
	Super::BeginPlay();
	Zone->OnComponentBeginOverlap.AddDynamic(this, &AControlArea::OnOverlapBegin);
	Zone->OnComponentEndOverlap.AddDynamic(this, &AControlArea::OnOverlapEnd);
	Zone->SetGenerateOverlapEvents(true);

	CreateDynamicMaterial();
	UpdateFlickerMaterial(); // 初始应用一次（bActive 默认 false）
}

void AControlArea::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	TickControl(DeltaSeconds);
}

void AControlArea::SetActive(bool bNewActive)
{
	if (!HasAuthority()) return;
	if (bActive == bNewActive) return;
	bActive = bNewActive;
	UpdateFlickerMaterial();

	if (bNewActive)
	{
		// 激活时重置占点进度（每轮从零开始），并重新收集当前已在圈内的玩家
		//（避免「点关闭后一直站着、再激活却不判定」）
		ControllingTeam = ETeam::None;
		ScoreProgress = 0.f;
		ControlProgress = 0.f;
		ControlAttemptTeam = ETeam::None;

		PlayersInZone.Reset();
		TArray<AActor*> Overlapping;
		Zone->GetOverlappingActors(Overlapping, As1mpleFpsCharacter::StaticClass());
		for (AActor* A : Overlapping)
		{
			if (As1mpleFpsCharacter* P = Cast<As1mpleFpsCharacter>(A))
			{
				PlayersInZone.AddUnique(P);
			}
		}

		MulticastPlayActivationSound();
	}
	else
	{
		PlayersInZone.Reset();
		ScoreProgress = 0.f;
		ControlProgress = 0.f;
		ControlAttemptTeam = ETeam::None;
	}
}

ETeam AControlArea::GetBiggestTeam()
{
	int32 Blue = 0, Red = 0;
	for (const As1mpleFpsCharacter* Player : PlayersInZone)
	{
		if (!Player || Player->IsDead()) continue;
		const As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Player->GetPlayerState());
		if (!PS) continue;
		if (PS->Team == ETeam::Blue) Blue++;
		else if (PS->Team == ETeam::Red) Red++;
	}
	if (Blue == Red) return ETeam::None;
	return Blue > Red ? ETeam::Blue : ETeam::Red;
}

void AControlArea::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AControlArea, bActive);
	DOREPLIFETIME(AControlArea, ControllingTeam);
	DOREPLIFETIME(AControlArea, ScoreProgress);
	DOREPLIFETIME(AControlArea, ControlProgress);
	DOREPLIFETIME(AControlArea, ControlAttemptTeam);
}

void AControlArea::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	As1mpleFpsCharacter* Player = Cast<As1mpleFpsCharacter>(OtherActor);
	if (!Player) return;

	PlayersInZone.RemoveAll([](As1mpleFpsCharacter* P) { return P == nullptr || P->IsDead(); });
	PlayersInZone.AddUnique(Player);
}

void AControlArea::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;
	As1mpleFpsCharacter* Player = Cast<As1mpleFpsCharacter>(OtherActor);
	if (!Player) return;

	PlayersInZone.Remove(Player);
}

ETeam AControlArea::GetMajorityTeam() const
{
	int32 Blue = 0, Red = 0;
	for (const As1mpleFpsCharacter* Player : PlayersInZone)
	{
		if (!Player || Player->IsDead()) continue;
		const As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Player->GetPlayerState());
		if (!PS) continue;
		if (PS->Team == ETeam::Blue) Blue++;
		else if (PS->Team == ETeam::Red) Red++;
	}
	if (Blue == 0 && Red == 0) return ETeam::None;
	if (Blue == Red) return ETeam::None;
	return Blue > Red ? ETeam::Blue : ETeam::Red;
}

bool AControlArea::IsContested() const
{
	bool bBlue = false, bRed = false;
	for (const As1mpleFpsCharacter* Player : PlayersInZone)
	{
		if (!Player || Player->IsDead()) continue;
		const As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Player->GetPlayerState());
		if (!PS) continue;
		if (PS->Team == ETeam::Blue) bBlue = true;
		else if (PS->Team == ETeam::Red) bRed = true;
	}
	return bBlue && bRed;
}

void AControlArea::TickControl(float DeltaSeconds)
{
	if (!HasAuthority() || !bActive) return;

	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (!GS || !GS->bMatchStarted || GS->bMatchEnded) return;

	PlayersInZone.RemoveAll([](As1mpleFpsCharacter* P) { return P == nullptr || P->IsDead(); });

	const ETeam Majority = GetMajorityTeam();

	// 空圈或平局：换边进度清零；得分进度冻结（不清零，等有人回来继续）
	if (Majority == ETeam::None)
	{
		ControlProgress = 0.f;
		ControlAttemptTeam = ETeam::None;
		return;
	}

	// 当前占领方仍是多数 → 走得分时间
	if (Majority == ControllingTeam)
	{
		ControlProgress = 0.f;
		ControlAttemptTeam = ETeam::None;
		ScoreProgress += DeltaSeconds / FMath::Max(ScoreTime, 0.01f);
		if (ScoreProgress >= 1.f)
		{
			AwardCapture(Majority);
		}
		return;
	}

	// Majority != ControllingTeam：新的一方想接管
	// 中性点且无争夺（圈内只有单队）→ 直接占领并开始得分（无需占领时间）
	if (ControllingTeam == ETeam::None && !IsContested())
	{
		ControllingTeam = Majority;
		ScoreProgress = 0.f;
		ControlProgress = 0.f;
		ControlAttemptTeam = ETeam::None;
		return;
	}

	// 争夺中的中性点，或敌方正在抢已有点 → 走占领时间（得分进度不清零，冻结）
	// 人多的一方变了 → 重走占领时间
	if (ControlAttemptTeam != Majority)
	{
		ControlAttemptTeam = Majority;
		ControlProgress = 0.f;
	}
	ControlProgress += DeltaSeconds / FMath::Max(ControlTime, 0.01f);
	if (ControlProgress >= 1.f)
	{
		ControllingTeam = Majority;   // 换边成功：被抢走
		ControlProgress = 0.f;
		ControlAttemptTeam = ETeam::None;
		ScoreProgress = 0.f;          // 被抢走 → 得分清零，新占领方从零开始
	}
}

void AControlArea::AwardCapture(ETeam Team)
{
	if (GetNetMode() == NM_Client) return;

	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (!GS) return;

	As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>();
	int32 Score = GM ? GM->ComputeControlScore(AwardScore) : FMath::RoundToInt(AwardScore);
	GS->AddTeamScore(Team, Score);
	GS->MulticastControlPointScored(Team, Score); // 广播得分队伍 + 得分（服务器 + 所有客户端）
	MulticastPlayCapturedSound();                 // 被占得分音效（服务器 + 所有客户端）

	if (GM)
	{
		GM->CheckWinnerCondition(Team);
		GM->OnControlPointCaptured(this, Team); // 关闭当前点 + 排下一轮
	}
	else
	{
		bActive = false; // 无调度器时自关，防止重复刷分
	}

	OnPointCaptured(Team);
}

void AControlArea::OnRep_ActiveChanged()
{
	OnPointActivated(bActive);
	UpdateFlickerMaterial(); // 客户端收到复制后同步材质
}

void AControlArea::OnRep_CaptureState()
{
	OnCaptureProgressChanged(ControllingTeam, ControlAttemptTeam, ScoreProgress, ControlProgress);
}

void AControlArea::MulticastPlayActivationSound_Implementation()
{
	if (ActivationSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ActivationSound, GetActorLocation());
	}
}

void AControlArea::MulticastPlayCapturedSound_Implementation()
{
	if (CapturedSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, CapturedSound, GetActorLocation());
	}
}

void AControlArea::CreateDynamicMaterial()
{
	if (!StaticMesh) return;

	UMaterialInterface* Material = StaticMesh->GetMaterial(MaterialSlotIndex);
	if (!Material)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ControlArea] %s 材质槽 %d 没有材质"), *GetName(), MaterialSlotIndex);
		return;
	}

	DynamicMaterialInstance = StaticMesh->CreateDynamicMaterialInstance(MaterialSlotIndex, Material);
}

void AControlArea::UpdateFlickerMaterial()
{
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("EnableFlicker"), bActive ? 1.0f : 0.0f);
	}
}
