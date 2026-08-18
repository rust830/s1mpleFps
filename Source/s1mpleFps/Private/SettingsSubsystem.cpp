// Copyright Epic Games, Inc. All Rights Reserved.

#include "SettingsSubsystem.h"
#include "SettingsSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "Engine/GameInstance.h"

static const FString SettingsSlotName = TEXT("PlayerSettings");

USettingsSubsystem* USettingsSubsystem::GetSettingsSubsystem(const UObject* WorldContextObject)
{
	UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GI ? GI->GetSubsystem<USettingsSubsystem>() : nullptr;
}

void USettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadSettings();
}

void USettingsSubsystem::Deinitialize()
{
	SaveSettings();
	Super::Deinitialize();
}

void USettingsSubsystem::LoadSettings()
{
	Settings = Cast<USettingsSaveGame>(UGameplayStatics::LoadGameFromSlot(SettingsSlotName, 0));
	if (!Settings)
	{
		Settings = Cast<USettingsSaveGame>(UGameplayStatics::CreateSaveGameObject(USettingsSaveGame::StaticClass()));
	}
	ApplyAudioSettings();
}

void USettingsSubsystem::SaveSettings()
{
	if (Settings)
	{
		UGameplayStatics::SaveGameToSlot(Settings, SettingsSlotName, 0);
	}
}

float USettingsSubsystem::GetMouseSensitivity() const
{
	return Settings ? Settings->MouseSensitivity : 1.0f;
}

void USettingsSubsystem::SetMouseSensitivity(float Value)
{
	if (Settings)
	{
		Settings->MouseSensitivity = Value;
		SaveSettings();
	}
}

void USettingsSubsystem::SetMasterVolume(float Value)
{
	if (Settings)
	{
		Settings->MasterVolume = Value;
		ApplyAudioSettings();
		SaveSettings();
	}
}

void USettingsSubsystem::SetSFXVolume(float Value)
{
	if (Settings)
	{
		Settings->SFXVolume = Value;
		ApplyAudioSettings();
		SaveSettings();
	}
}

void USettingsSubsystem::SetUIVolume(float Value)
{
	if (Settings)
	{
		Settings->UIVolume = Value;
		ApplyAudioSettings();
		SaveSettings();
	}
}

float USettingsSubsystem::GetMasterVolume() const { return Settings ? Settings->MasterVolume : 1.0f; }
float USettingsSubsystem::GetSFXVolume() const { return Settings ? Settings->SFXVolume : 1.0f; }
float USettingsSubsystem::GetUIVolume() const { return Settings ? Settings->UIVolume : 1.0f; }

void USettingsSubsystem::ApplyAllSettings()
{
	ApplyAudioSettings();
}

void USettingsSubsystem::ApplyAudioSettings()
{
	if (!Settings)
	{
		return;
	}

	USoundMix* Mix = MasterSoundMix.LoadSynchronous();
	if (!Mix)
	{
		return;
	}

	UObject* WorldContext = GetGameInstance() ? Cast<UObject>(GetGameInstance()->GetWorld()) : nullptr;
	if (!WorldContext)
	{
		WorldContext = GetGameInstance();
	}

	if (MasterSoundClass.IsValid())
	{
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, Mix, MasterSoundClass.LoadSynchronous(), Settings->MasterVolume, 1.0f, 0.0f, true);
	}
	if (SFXSoundClass.IsValid())
	{
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, Mix, SFXSoundClass.LoadSynchronous(), Settings->SFXVolume, 1.0f, 0.0f, true);
	}
	if (UISoundClass.IsValid())
	{
		UGameplayStatics::SetSoundMixClassOverride(WorldContext, Mix, UISoundClass.LoadSynchronous(), Settings->UIVolume, 1.0f, 0.0f, true);
	}
}
