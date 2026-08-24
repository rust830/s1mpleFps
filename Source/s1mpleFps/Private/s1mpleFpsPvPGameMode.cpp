#include "s1mpleFpsPvPGameMode.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsGameInstance.h"
#include "s1mpleFpsPlayerController.h"
#include "s1mpleFpsCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"

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
	As1mpleFpsGameState* GS = GetGameState<As1mpleFpsGameState>();

	// 结算击杀者
	if (Killer && Killer != Victim)
	{
		// 团队记分：友军击杀不计团队分（个人 Kills/钱照常结算）
		ETeam KillerTeam = Killer->Team;
		bool bSameTeam = Victim && KillerTeam != ETeam::None && KillerTeam == Victim->Team;
		if (GS && !bSameTeam && KillerTeam != ETeam::None)
		{
			GS->AddTeamKill(KillerTeam); // 先记团队分，AddKill 内部的 CheckWinnerCondition 才读得到最新团队分
		}

		int32 Reward = CalculateKillReward(Killer);
		Killer->AddKill();
		Killer->IncrementKillStreak();
		Killer->AddMoney(Reward);

		// 击杀/连杀音效：只发给击杀者本人（此时 KillStreak 已自增）
		if (As1mpleFpsCharacter* KillerChar = Cast<As1mpleFpsCharacter>(Killer->GetPawn()))
		{
			const int32 Streak = Killer->KillStreak;
			USoundBase* Sound = nullptr;
			if (Streak >= 2 && MultiKillSounds.Num() > 0)
			{
				const int32 Idx = FMath::Clamp(Streak - 2, 0, MultiKillSounds.Num() - 1);
				Sound = MultiKillSounds[Idx];
			}
			else
			{
				Sound = KillSound;
			}
			if (Sound)
			{
				KillerChar->ClientPlayKillSound(Sound);
			}
		}
	}

	// 结算被击杀者
	if (Victim)
	{
		Victim->AddDeath();
		Victim->IncrementDeathStreak();
	}

	if (GS && Killer && Victim) {
		const FString& KillerName = Killer->GetPlayerName();
		const FString& VictimName = Victim->GetPlayerName();
		GS->MulticastKillPlay(KillerName, VictimName, Killer->Team);
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

void As1mpleFpsPvPGameMode::RestoreCarriedPlayer(AController* NewPC)
{
	if (!NewPC) return;

	As1mpleFpsPlayerState* PS = NewPC->GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS) return;

	Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>();
	if (!GI) return;

	// 恢复大厅里选好的队伍
	if (PS->Team == ETeam::None)
	{
		ETeam Team = GI->GetPlayerTeam(PS->GetUniqueId());
		if (Team != ETeam::None)
		{
			PS->Team = Team;
		}
	}

	// 恢复选中英雄索引（未选过则返回 0 = 默认第一个英雄）
	PS->SelectedHeroIndex = GI->GetPlayerHero(PS->GetUniqueId());
	PS->ForceNetUpdate();

	UE_LOG(LogTemp, Log, TEXT("[Hero] 恢复携带数据: hero=%d team=%d, Player=%s"),
		PS->SelectedHeroIndex, (int32)PS->Team, *PS->GetPlayerName());
}

void As1mpleFpsPvPGameMode::PostLogin(APlayerController* NewPC)
{
	// 先恢复大厅里选好的队伍 + 英雄（在 Super::PostLogin 之前，这样生成 Pawn 时已能拿到正确队伍/英雄）
	RestoreCarriedPlayer(NewPC);
	Super::PostLogin(NewPC);
}

void As1mpleFpsPvPGameMode::HandleSeamlessTravelPlayer(AController*& C)
{
	// 先让引擎换 Controller + 重建 PlayerState + 生成 Pawn。
	// 注意：PlayerState 是在 Super 内部才重建的（InitSeamlessTravelPlayer），
	// 所以恢复必须放在 Super 之后，否则会写到旧 PlayerState 上（角色读的是重建后的那个）。
	Super::HandleSeamlessTravelPlayer(C);
	RestoreCarriedPlayer(C);
}

AActor* As1mpleFpsPvPGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	// 选出生点前先恢复队伍 + 英雄。这里处于「PlayerState 重建后、Pawn 生成前」，
	// 是无缝跳图里恢复 Team 的唯一切合时机的点（否则 PS->Team 还是 None，只能落到默认出生点）。
	// 普通登录路径下幂等，不会破坏 PostLogin 已恢复好的值。
	RestoreCarriedPlayer(Player);

	// 根据玩家队伍选出生点（PlayerStart 的 PlayerStartTag 标 "Blue"/"Red"）
	ETeam Team = ETeam::None;
	if (Player && Player->PlayerState)
	{
		if (As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Player->PlayerState))
		{
			Team = PS->Team;
		}
	}

	if (Team != ETeam::None)
	{
		const FName TeamTag = (Team == ETeam::Blue) ? FName(TEXT("Blue")) : FName(TEXT("Red"));
		TArray<APlayerStart*> TeamStarts;
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			APlayerStart* Start = *It;
			if (Start && Start->PlayerStartTag == TeamTag)
			{
				TeamStarts.Add(Start);
			}
		}
		if (TeamStarts.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[PvP] 队伍 %d 找到 %d 个出生点"), (int32)Team, TeamStarts.Num());
			return TeamStarts[FMath::RandRange(0, TeamStarts.Num() - 1)];
		}
		UE_LOG(LogTemp, Warning, TEXT("[PvP] 队伍 %d 没找到带 PlayerStartTag 的出生点，回退默认出生点"), (int32)Team);
	}

	// 没找到对应队伍出生点（或队伍未定），回退默认
	AActor* Start = Super::FindPlayerStart_Implementation(Player, IncomingName);
	UE_LOG(LogTemp, Warning, TEXT("[PvP] 回退默认出生点: %s"), Start ? *Start->GetName() : TEXT("NULL(地图里没有任何 PlayerStart!)"));
	return Start;
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
	if (!PS) return;
	ETeam Team = PS->Team;
	if (Team == ETeam::None) return;

	As1mpleFpsGameState* GS = GetGameState<As1mpleFpsGameState>();
	if (!GS || GS->bMatchEnded) return;

	// 常规：团队总击杀达标
	if (GS->GetTeamKills(Team) >= KillLimits) {
		GS->AnnounceWinner(GS->GetTeamName(Team), true);
		return;
	}
	// 加时：团队在加时内新增击杀达标
	if (GS->bSuddenDeath) {
		int32 OTKills = GS->GetTeamKills(Team) - GS->GetOvertimeStartTeamKills(Team);
		if (OTKills >= OvertimeKillTargets) {
			GS->AnnounceWinner(GS->GetTeamName(Team), true);
		}
	}
}
