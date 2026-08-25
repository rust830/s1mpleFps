// Fill out your copyright notice in the Description page of Project Settings.


#include "ControlArea.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "s1mpleFpsCharacter.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPvPGameMode.h"
#include "Net/UnrealNetwork.h"

AControlArea::AControlArea()
{
	PrimaryActorTick.bCanEverTick = false;
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
}

void AControlArea::SetActive(bool bNewActive)
{
	if (!HasAuthority()) return;
	if (bActive == bNewActive) return;
	bActive = bNewActive;

	if (bNewActive)
	{
		// 激活时重新收集当前已在圈内的玩家（避免“点关闭后一直站着、再激活却不判定”）
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
		TryCapture();
	}
	else
	{
		PlayersInZone.Reset();
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
}

void AControlArea::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	As1mpleFpsCharacter* Player = Cast<As1mpleFpsCharacter>(OtherActor);
	if (!Player) return;

	PlayersInZone.RemoveAll([](As1mpleFpsCharacter* P) { return P == nullptr || P->IsDead(); });
	PlayersInZone.AddUnique(Player);
	TryCapture();
}

void AControlArea::OnOverlapEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority()) return;
	As1mpleFpsCharacter* Player = Cast<As1mpleFpsCharacter>(OtherActor);
	if (!Player) return;

	PlayersInZone.Remove(Player);
	TryCapture();
}

void AControlArea::TryCapture()
{
	if (!HasAuthority() || !bActive) return;

	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (!GS || !GS->bMatchStarted || GS->bMatchEnded) return;

	PlayersInZone.RemoveAll([](As1mpleFpsCharacter* P) { return P == nullptr || P->IsDead(); });

	ETeam Team = GetSoleTeam();
	if (Team != ETeam::None)
	{
		AwardCapture(Team);
	}
}

ETeam AControlArea::GetSoleTeam() const
{
	ETeam Sole = ETeam::None;
	for (const As1mpleFpsCharacter* Player : PlayersInZone)
	{
		if (!Player || Player->IsDead()) continue;
		const As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Player->GetPlayerState());
		if (!PS || PS->Team == ETeam::None) continue;
		if (Sole == ETeam::None) Sole = PS->Team;
		else if (Sole != PS->Team) return ETeam::None;
	}
	return Sole;
}

void AControlArea::AwardCapture(ETeam Team)
{
	if (GetNetMode() == NM_Client) return;

	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (!GS) return;

	As1mpleFpsPvPGameMode* GM = GetWorld()->GetAuthGameMode<As1mpleFpsPvPGameMode>();
	int32 Score = GM ? GM->ComputeControlScore(AwardScore) : FMath::RoundToInt(AwardScore);
	GS->AddTeamScore(Team, Score);

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
}
