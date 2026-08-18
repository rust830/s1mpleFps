// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SettingsSubsystem.generated.h"

class USettingsSaveGame;
class USoundMix;
class USoundClass;

// 全局设置子系统：统一管理鼠标灵敏度 + 主/音效/UI 音量，读写 USettingsSaveGame 存档
UCLASS()
class S1MPLEFPS_API USettingsSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 存档读写
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadSettings();
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	// 鼠标灵敏度
	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMouseSensitivity() const;
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMouseSensitivity(float Value);

	// 音量
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetMasterVolume(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetSFXVolume(float Value);
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SetUIVolume(float Value);
	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetMasterVolume() const;
	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetSFXVolume() const;
	UFUNCTION(BlueprintPure, Category = "Settings")
	float GetUIVolume() const;

	// 一次性应用全部设置（音量等）。WBP_Settings 的「应用」按钮调用这个。
	UFUNCTION(BlueprintCallable, Category = "Settings")
	void ApplyAllSettings();

	// 音频资产引用：在编辑器里给子系统指定 SoundMix / SoundClass（Master/SFX/UI）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundMix> MasterSoundMix;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> MasterSoundClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> SFXSoundClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Audio")
	TSoftObjectPtr<USoundClass> UISoundClass;

private:
	UPROPERTY()
	USettingsSaveGame* Settings;

	void ApplyAudioSettings();
};
