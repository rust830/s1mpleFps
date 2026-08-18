// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSaveGame.generated.h"

// 设置存档：存灵敏度/音量。键位重绑交给 Player Mappable Input（不在这里手写 TMap）
UCLASS()
class S1MPLEFPS_API USettingsSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// 存档数据由 USettingsSubsystem 内部读写，不暴露给蓝图（避免和 Subsystem 的同名函数冲突）
	UPROPERTY(EditAnywhere, Category = "Settings")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Settings")
	float MasterVolume = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Settings")
	float SFXVolume = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Settings")
	float UIVolume = 1.0f;
};
