// Fill out your copyright notice in the Description page of Project Settings.


#include "s1mpleFpsGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"

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
	TravelMapName = LobbyName;

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

void Us1mpleFpsGameInstance::JoinSession(int32 SessionIndex)
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
