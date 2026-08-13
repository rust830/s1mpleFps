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
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] OSS=%s, SessionInterface=%s"),
			*Subsystem->GetSubsystemName().ToString(),
			SessionInterface.IsValid() ? TEXT("Valid") : TEXT("NULL"));
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] No OnlineSubsystem found!"));
	}
}

void Us1mpleFpsGameInstance::HostGame(const FString& SessionName, int32 MaxPlayers, bool bIsLAN)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] HostGame called: SessionName=%s, MaxPlayers=%d, bIsLAN=%d, SessionInterface=%s"),
		*SessionName, MaxPlayers, bIsLAN, SessionInterface.IsValid() ? TEXT("Valid") : TEXT("NULL"));

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
	TravelMapName = SessionName;

	SessionInterface->CreateSession(0, NAME_GameSession, *Settings);
}

void Us1mpleFpsGameInstance::FindSession(int32 MaxSearchResults, bool bIsLAN)
{
	if (!SessionInterface.IsValid()) return;
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] FindSession: MaxResults=%d, bIsLAN=%d"), MaxSearchResults, bIsLAN);

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
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] JoinSession called: Index=%d, SessionInterface=%s, LastSearchResult=%s"),
		SessionIndex,
		SessionInterface.IsValid() ? TEXT("Valid") : TEXT("NULL"),
		LastSearchResult.IsValid() ? TEXT("Valid") : TEXT("NULL"));

	if (!SessionInterface.IsValid()) return;
	if (!LastSearchResult.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] JoinSession failed: LastSearchResult is NULL"));
		return;
	}
	if (SessionIndex < 0 || SessionIndex >= LastSearchResult->SearchResults.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] JoinSession failed: Index=%d out of range (0-%d)"),
			SessionIndex, LastSearchResult->SearchResults.Num() - 1);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] JoinSession: joining session #%d, Owner=%s"),
		SessionIndex, *LastSearchResult->SearchResults[SessionIndex].Session.OwningUserName);

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

void Us1mpleFpsGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	bIsHosting = false;
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] OnCreateSessionComplete: SessionName=%s, Success=%d"),
		*SessionName.ToString(), bWasSuccessful);
	if (bWasSuccessful) {
		UWorld* World = GetWorld();
		if (World) {
			UE_LOG(LogTemp, Warning, TEXT("[GameInstance] ServerTravel: %s?Listen"), *TravelMapName);
			World->ServerTravel(TravelMapName + TEXT("?Listen"));
		}
	}
}

void Us1mpleFpsGameInstance::OnFindSessionCompleteInternal(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] OnFindSessionComplete: Success=%d, ResultCount=%d"),
		bWasSuccessful,
		LastSearchResult.IsValid() ? LastSearchResult->SearchResults.Num() : -1);
	OnFindSessionComplete.Broadcast();
}

void Us1mpleFpsGameInstance::OnJoinSessionCompleteInternal(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[GameInstance] OnJoinSessionComplete: SessionName=%s, Result=%d"),
		*InSessionName.ToString(), (int32)Result);

	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	if (bSuccess) {
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(InSessionName, ConnectString)) {
			UE_LOG(LogTemp, Warning, TEXT("[GameInstance] Join success, ConnectString=%s"), *ConnectString);
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
			if (PC) {
				PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("[GameInstance] Join failed: PlayerController is NULL"));
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("[GameInstance] Join failed: GetResolvedConnectString returned false"));
		}
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("[GameInstance] Join failed: Result=%d is not Success"), (int32)Result);
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
