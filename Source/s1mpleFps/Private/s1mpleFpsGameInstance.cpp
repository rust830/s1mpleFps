// Fill out your copyright notice in the Description page of Project Settings.


#include "s1mpleFpsGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Misc/PackageName.h"
#include "StreamlineLibraryReflex.h"

void Us1mpleFpsGameInstance::Init()
{
	Super::Init();
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (Subsystem) {
		SessionInterface = Subsystem->GetSessionInterface();
		
	}
	else {
		
	}
}

void Us1mpleFpsGameInstance::OnStart()
{
	Super::OnStart();

	// RTX 4070 Laptop：帧生成 + Reflex 低延迟降低输入延迟，笔记本上尤其有用。
	// 默认 Enabled（低延迟）；GPU 有余量可改 Boost（更低延迟但功耗/发热略增）。
	if (UStreamlineLibraryReflex::IsReflexSupported())
	{
		UStreamlineLibraryReflex::SetReflexMode(EStreamlineReflexMode::Enabled);
		UE_LOG(LogTemp, Log, TEXT("[DLSS] NVIDIA Reflex 低延迟已开启 (Enabled)"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[DLSS] 当前硬件/驱动不支持 Reflex，已跳过"));
	}
}

void Us1mpleFpsGameInstance::HostGame(const FString& SessionName,const FString& LobbyName, int32 MaxPlayers, bool bIsLAN)
{
	

	if (bIsHosting) return;
	if (!SessionInterface.IsValid()) return;
	bIsHosting = true;
	FNamedOnlineSession* Exist = SessionInterface->GetNamedSession(NAME_GameSession);
	if (Exist) {
		SessionInterface->DestroySession(NAME_GameSession);
	}

	FOnCreateSessionCompleteDelegate CreateDelegate;
	CreateDelegate.BindUObject(this, &Us1mpleFpsGameInstance::OnCreateSessionComplete);
	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(CreateDelegate);

	TSharedPtr<FOnlineSessionSettings> Settings = MakeShareable(new FOnlineSessionSettings());
	Settings->bIsLANMatch = bIsLAN;
	Settings->bUsesPresence = !bIsLAN;
	Settings->bShouldAdvertise = true;
	Settings->bAllowJoinInProgress = true;
	Settings->NumPublicConnections = MaxPlayers;
	Settings->Set(FName(TEXT("MAPNAME")), SessionName, EOnlineDataAdvertisementType::ViaOnlineService);
	TravelMapName = ResolveMapPath(LobbyName);

	SessionInterface->CreateSession(0, NAME_GameSession, *Settings);
}

void Us1mpleFpsGameInstance::FindSession(int32 MaxSearchResults, bool bIsLAN)
{
	if (!SessionInterface.IsValid()) return;
	

	LastSearchResult = MakeShareable(new FOnlineSessionSearch());
	LastSearchResult->MaxSearchResults = MaxSearchResults;
	LastSearchResult->bIsLanQuery = bIsLAN;

	FOnFindSessionsCompleteDelegate FindDelegate;
	FindDelegate.BindUObject(this, &Us1mpleFpsGameInstance::OnFindSessionCompleteInternal);
	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FindDelegate);

	SessionInterface->FindSessions(0, LastSearchResult.ToSharedRef());
}

void Us1mpleFpsGameInstance::MyJoinSession(int32 SessionIndex)
{
	

	if (!SessionInterface.IsValid()) return;
	if (!LastSearchResult.IsValid())
	{
		
		return;
	}
	if (SessionIndex < 0 || SessionIndex >= LastSearchResult->SearchResults.Num())
	{
		
		return;
	}

	

	FOnJoinSessionCompleteDelegate JoinDelegate;
	JoinDelegate.BindUObject(this, &Us1mpleFpsGameInstance::OnJoinSessionCompleteInternal);
	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(JoinDelegate);

	SessionInterface->JoinSession(0, NAME_GameSession, LastSearchResult->SearchResults[SessionIndex]);
}

void Us1mpleFpsGameInstance::LeaveSession()
{
	if (!SessionInterface.IsValid()) return;

	FOnDestroySessionCompleteDelegate DestroyDelegate;
	DestroyDelegate.BindUObject(this, &Us1mpleFpsGameInstance::OnDestroySessionComplete);
	SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(DestroyDelegate);

	SessionInterface->DestroySession(NAME_GameSession);
}

void Us1mpleFpsGameInstance::SetSessionJoinable(bool bJoinable)
{
	if (!SessionInterface.IsValid())return;
	FOnlineSessionSettings* SessionSettings = SessionInterface->GetSessionSettings(NAME_GameSession);
	if (SessionSettings) {
		SessionSettings->bAllowJoinInProgress=bJoinable;
		SessionInterface->UpdateSession(NAME_GameSession, *SessionSettings, true);
	}
}

void Us1mpleFpsGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	bIsHosting = false;
	
	if (bWasSuccessful) {
		UWorld* World = GetWorld();
		if (World) {
			
			World->ServerTravel(TravelMapName + TEXT("?Listen"));
		}
	}
}

void Us1mpleFpsGameInstance::OnFindSessionCompleteInternal(bool bWasSuccessful)
{
	
	OnFindSessionComplete.Broadcast();
}

void Us1mpleFpsGameInstance::OnJoinSessionCompleteInternal(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	

	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	if (bSuccess) {
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(InSessionName, ConnectString)) {
			
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			if (PC) {
				PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
			else {
				
			}
		}
		else {
			
		}
	}
	else {
		
	}
	OnJoinSessionComplete.Broadcast(bSuccess);
}

void Us1mpleFpsGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		PC->ClientTravel(TEXT("/Game/FirstPerson/Maps/StartMap"), ETravelType::TRAVEL_Absolute);
	}
}

FString Us1mpleFpsGameInstance::ResolveMapPath(const FString& MapName)
{
	// 已经带路径前缀（/Game/...、/Engine/...）就直接用，别再处理
	if (MapName.StartsWith(TEXT("/")))
	{
		return MapName;
	}

	// 短名 → 完整包路径。PIE 里编辑器会自动解析短名，打包后 ServerTravel 不认，
	// 会打 "Can't Find URL" 然后回退到 GameDefaultMap（StartMap）。
	FString FullPath;
	if (FPackageName::SearchForPackageOnDisk(MapName, &FullPath))
	{
		UE_LOG(LogTemp, Log, TEXT("[Host] 地图短名 %s → %s"), *MapName, *FullPath);
		return FullPath;
	}

	// 兜底：项目地图都在 /Game/FirstPerson/Maps/ 下（见 GameMapsSettings）
	FullPath = FString::Printf(TEXT("/Game/FirstPerson/Maps/%s"), *MapName);
	UE_LOG(LogTemp, Warning, TEXT("[Host] 未找到地图 %s，按默认目录兜底为 %s"), *MapName, *FullPath);
	return FullPath;
}

int32 Us1mpleFpsGameInstance::GetSessionCount() const
{
	if (!LastSearchResult.IsValid()) return 0;
	return LastSearchResult->SearchResults.Num();
}

void Us1mpleFpsGameInstance::GetSessionInfo(int32 Index, FString& OutName, int32& OutPing, int32& OutCurrentPlayers, int32& OutMaxPlayers) const
{
	OutName = TEXT("");
	OutPing = 0;
	OutCurrentPlayers = 0;
	OutMaxPlayers = 0;

	if (!LastSearchResult.IsValid()) return;
	if (Index < 0 || Index >= LastSearchResult->SearchResults.Num()) return;

	const FOnlineSessionSearchResult& Result = LastSearchResult->SearchResults[Index];
	OutName = Result.Session.OwningUserName;
	OutPing = Result.PingInMs;
	OutCurrentPlayers = Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections;
	OutMaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
}

void Us1mpleFpsGameInstance::SetPlayerTeam(const FUniqueNetIdRepl& PlayerId, ETeam Team)
{
	PlayerTeams.Add(PlayerId, Team);
}

ETeam Us1mpleFpsGameInstance::GetPlayerTeam(const FUniqueNetIdRepl& PlayerId) const
{
	if (const ETeam* Found = PlayerTeams.Find(PlayerId))
	{
		return *Found;
	}
	return ETeam::None;
}

void Us1mpleFpsGameInstance::ClearPlayerTeams()
{
	PlayerTeams.Empty();
}

void Us1mpleFpsGameInstance::SetPlayerHero(const FUniqueNetIdRepl& PlayerId, int32 HeroIndex)
{
	PlayerHeroes.Add(PlayerId, HeroIndex);
}

int32 Us1mpleFpsGameInstance::GetPlayerHero(const FUniqueNetIdRepl& PlayerId) const
{
	if (const int32* Found = PlayerHeroes.Find(PlayerId))
	{
		return *Found;
	}
	return 0;
}

void Us1mpleFpsGameInstance::ClearPlayerHeroes()
{
	PlayerHeroes.Empty();
}

void Us1mpleFpsGameInstance::SetDesiredPlayerName(const FString& Name)
{
	DesiredPlayerName = Name;
}

FString Us1mpleFpsGameInstance::GetDesiredPlayerName() const
{
	return DesiredPlayerName;
}
