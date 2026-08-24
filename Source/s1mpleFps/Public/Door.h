// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Door.generated.h"


class UBoxComponent;
class UStaticMeshComponent;
class As1mpleFpsPlayerController;

UENUM(BlueprintType)
enum class EOpenType :uint8 {
	Rotate,
	Translate
};

UCLASS()
class S1MPLEFPS_API ADoor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADoor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	//Character interface
	void Door_Interact(APlayerController* Player);


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	EOpenType Type = EOpenType::Rotate;
protected:
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Door")
	TObjectPtr<UBoxComponent> Box;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	TObjectPtr<UStaticMeshComponent> DoorMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door",meta=(EditCondition="Type==EOpenType::Rotate"))
	float YawRotation = 120.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door", meta = (EditCondition = "Type==EOpenType::Translate"))
	FVector Location = FVector(200.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Door")
	float InterpSpeed = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Door")
	float MaxInteractDistance = 100.f;



	UPROPERTY(ReplicatedUsing=OnRep_DoorState)
	bool bIsOpen = false;
	UPROPERTY(ReplicatedUsing = OnRep_DoorState)
	bool bIsMoving = false;

	bool bMyLocalActive = false;
	UPROPERTY(Replicated)
	FRotator TargetRotation;
	UPROPERTY(Replicated)
	FVector TargetLocation;
	UFUNCTION(Server,Reliable)
	void ServerOpenDoor(As1mpleFpsPlayerController* PC);
	UFUNCTION()
	void OnRep_DoorState();

public:


	UPROPERTY(BlueprintReadOnly,Category="Door")
	As1mpleFpsPlayerController* InteractController;
	UFUNCTION()
	void OnTriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
};
