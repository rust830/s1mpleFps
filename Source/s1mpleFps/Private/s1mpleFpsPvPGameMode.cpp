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

	int32 Reward = KillRewardBase;
	// 连杀奖励：当前连杀数 × 每次加成
	Reward += Killer->KillStreak * KillStreakBonus;
	// 连死补偿：死亡多把后终于拿人头，额外补偿
	Reward += Killer->DeathStreak * DeathStreakBonusPerLevel;

	return Reward;
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

			UE_LOG(LogTemp, Warning, TEXT("[PvP] Match End �� %s: K=%d D=%d S=%d"),
				*PvPPS->GetPlayerName(), PvPPS->Kills, PvPPS->Deaths, PvPPS->Scores);
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
