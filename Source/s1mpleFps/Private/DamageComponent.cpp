// Fill out your copyright notice in the Description page of Project Settings.


#include "DamageComponent.h"

// Sets default values for this component's properties
UDamageComponent::UDamageComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UDamageComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	UE_LOG(LogTemp, Warning, TEXT("[DIAG] DamageComponent BeginPlay: Owner=%s, CurrentHealth=%.0f, MaxHealth=%.0f, Role=%d"),
		*GetNameSafe(GetOwner()), CurrentHealth, MaxHealth, (int32)GetOwnerRole());
}


// Called every frame
void UDamageComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	// ...
}

