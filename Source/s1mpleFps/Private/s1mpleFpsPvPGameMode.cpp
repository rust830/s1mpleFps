#include "s1mpleFpsPvPGameMode.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPlayerController.h"

As1mpleFpsPvPGameMode::As1mpleFpsPvPGameMode()
{
	// DefaultPawnClass and PlayerControllerClass are set in Blueprint (Class Defaults → Classes)

	PlayerStateClass = As1mpleFpsPlayerState::StaticClass();
	GameStateClass = As1mpleFpsGameState::StaticClass();
}

void As1mpleFpsPvPGameMode::OnKill(APlayerState* KillerPS, APlayerState* VictimPS)
{
	if (!HasAuthority()) return;

	As1mpleFpsPlayerState* Victim = Cast<As1mpleFpsPlayerState>(VictimPS);
	As1mpleFpsPlayerState* Killer = Cast<As1mpleFpsPlayerState>(KillerPS);

	// 结算击杀者
	if (Killer && Killer != Victim)
	{
		int32 Reward = CalculateKillReward(Killer);
		Killer->AddKill();
		Killer->IncrementKillStreak();
		Killer->AddMoney(Reward);
	}

	// 结算被击杀者
	if (Victim)
	{
		Victim->AddDeath();
		Victim->IncrementDeathStreak();
	}

	As1mpleFpsGameState* GS = GetGameState<As1mpleFpsGameState>();
	if (GS && Killer && Victim) {
		const FString& KillerName = Killer->GetPlayerName();
		const FString& VictimName = Victim->GetPlayerName();
		GS->MulticastKillPlay(KillerName, VictimName);
	}
}

int32 As1mpleFpsPvPGameMode::CalculateKillReward(As1mpleFpsPlayerState* Killer) const
{
	if (!Killer) return KillRewardBase;
	return ComputeKillReward(KillRewardBase, KillStreakBonus, DeathStreakBonusPerLevel, Killer->KillStreak, Killer->DeathStreak);
}

void As1mpleFpsPvPGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (As1mpleFpsGameState* GS = GetGameState<As1mpleFpsGameState>()) {
		GS->MatchTimeRemaining = MatchDuration;
		GS->StartCountdown();
	}
}

void As1mpleFpsPvPGameMode::OnMatchEnd()
{
	for (APlayerState* PS : GameState->PlayerArray) {
		As1mpleFpsPlayerState* PvPPS = Cast<As1mpleFpsPlayerState>(PS);
		if (PvPPS) {

			
		}
	}
}

void As1mpleFpsPvPGameMode::CheckWinnerCondition(As1mpleFpsPlayerState* PS)
{
	if (!PS)return;
	As1mpleFpsGameState* GS = GetGameState<As1mpleFpsGameState>();
	if (!GS || GS->bMatchEnded)return;
	if (PS->Kills >= KillLimits) {
		GS->AnnounceWinner(PS->GetPlayerName(), true);
		return;
	}
	if (GS->bSuddenDeath) {
		int32* StartKills = GS->OvertimeStartKills.Find(PS);
		int32 OTKills = StartKills ? (PS->Kills - *StartKills) : 0;
		if (OTKills >= OvertimeKillTargets) {
			GS->AnnounceWinner(PS->GetPlayerName(), true);
		}	
	}
}
