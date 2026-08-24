// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "HeroData.generated.h"

/**
 * 英雄/皮肤数据：一个 UHeroData 资产对应一个可选英雄（LOL/王者选英雄界面里的一个格子）。
 * 最少配 Mesh（第三人称全身模型）；DisplayName / Portrait 给选人 UI 用；AnimInstanceClass 可选。
 */
UCLASS(BlueprintType)
class S1MPLEFPS_API UHeroData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	// 选人界面显示的名字
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	FText DisplayName;

	// 选人界面头像
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	UTexture2D* Portrait = nullptr;

	// 第三人称全身模型（核心：进游戏后换这个骨骼网格）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	USkeletalMesh* Mesh = nullptr;

	// 可选：英雄专属 AnimInstance（动画蓝图），留空则用角色默认动画蓝图
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Hero")
	TSubclassOf<UAnimInstance> AnimInstanceClass = nullptr;
};
