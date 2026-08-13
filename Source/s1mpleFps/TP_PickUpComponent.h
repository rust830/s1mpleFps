// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "s1mpleFpsCharacter.h"
#include "Net/UnrealNetwork.h"
#include "TP_PickUpComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickUp, As1mpleFpsCharacter*, PickUpCharacter);

UCLASS(Blueprintable, BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class S1MPLEFPS_API UTP_PickUpComponent : public USphereComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FOnPickUp OnPickUp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Pickup")
	bool bIsAlreadyPickedUp = false;

	UTP_PickUpComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};