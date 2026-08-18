#include "BPFL_GameUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"

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

// 从 WorldContext 取 Enhanced Input User Settings（Player Mappable Input）
static UEnhancedInputUserSettings* GetUserSettingsForContext(UObject* WorldContext)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::LogAndReturnNull);
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	ULocalPlayer* LP = GI ? GI->GetFirstGamePlayer() : nullptr;
	UEnhancedInputLocalPlayerSubsystem* Subsystem = LP ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP) : nullptr;
	return Subsystem ? Subsystem->GetUserSettings() : nullptr;
}

bool UBPFL_GameUtils::RemapKey(UObject* WorldContext, FName MappingName, FKey NewKey, FString& OutError)
{
	OutError = TEXT("");
	UEnhancedInputUserSettings* Settings = GetUserSettingsForContext(WorldContext);
	if (!Settings)
	{
		OutError = TEXT("无法获取 Enhanced Input User Settings");
		return false;
	}

	FMapPlayerKeyArgs Args;
	Args.MappingName = MappingName;
	Args.NewKey = NewKey;
	Args.bCreateMatchingSlotIfNeeded = true;

	FGameplayTagContainer FailureReason;
	Settings->MapPlayerKey(Args, FailureReason);

	if (FailureReason.Num() > 0)
	{
		OutError = FString::Printf(TEXT("改键失败，错误标签 %d 个"), FailureReason.Num());
		return false;
	}
	return true;
}
