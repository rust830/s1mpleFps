#include "BPFL_GameUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

void UBPFL_GameUtils::RestartGame(UObject* WorldContext)
{
	if (!WorldContext) return;

	// OpenLevel 前重置输入模式，防止死亡界面的 UIOnly 模式残留导致重启后无法操作
	if (UWorld* World = WorldContext->GetWorld())
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			FInputModeGameOnly InputMode;
			PC->SetInputMode(InputMode);
			PC->bShowMouseCursor = false;
		}
	}

	FString LevelName = UGameplayStatics::GetCurrentLevelName(WorldContext, true);
	UGameplayStatics::OpenLevel(WorldContext, FName(*LevelName), false);
}

void UBPFL_GameUtils::QuitToDesktop(UObject* WorldContext)
{
	if (!WorldContext) return;
	APlayerController* PC = WorldContext->GetWorld()->GetFirstPlayerController();
	UKismetSystemLibrary::QuitGame(WorldContext, PC, EQuitPreference::Quit, false);
}
void UBPFL_GameUtils::HostGame(UObject* WorldContext, const FString& MapName)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	if (World)
	{
		World->ServerTravel(MapName + TEXT("?listen"));
	}
}

void UBPFL_GameUtils::JoinGame(UObject* WorldContext, const FString& IPAddress)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(WorldContext, 0);
	if (PC)
	{
		PC->ClientTravel(IPAddress, ETravelType::TRAVEL_Absolute);
	}
}
