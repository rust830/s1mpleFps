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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MasterVolume = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float SFXVolume = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float UIVolume = 1.0f;
};
