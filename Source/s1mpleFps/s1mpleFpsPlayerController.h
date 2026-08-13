// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BuyEquipmentData.h"
#include "s1mpleFpsPlayerController.generated.h"

class UChatWidget;



class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UHUDWidget;
class UBuyEquipmentData;
class As1mpleFpsCharacter;

UCLASS()
class S1MPLEFPS_API As1mpleFpsPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputMappingContext* InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UUserWidget> PauseMenuClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UUserWidget> PauseMenuWidget;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UChatWidget> ChatClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UChatWidget> ChatWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* ScoreboardAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* BuyMenuAction;

	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UInputAction* OpenTeamChat;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UInputAction* OpenAllChat;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* EscapeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Buy")
	TObjectPtr<UBuyEquipmentData> BuyEquipmentData;

	UPROPERTY()
		bool bChatOpen=false;
	UPROPERTY(BlueprintReadOnly)
		bool bCurrentChatIsTeam = false;
	UFUNCTION(BlueprintCallable)
		void OpenChatBox(bool bIsTeam);
	UFUNCTION(BlueprintCallable)
		void CloseChatBox();
	UFUNCTION(BlueprintCallable)
		void OnEscapePressed();

	UFUNCTION(BlueprintCallable)
		const TArray<FBuyItemEntry>& GetBuyItems() const;
	UFUNCTION(BlueprintCallable)
		int32 GetCurrentMoney() const;
	UFUNCTION(BlueprintCallable)
		void RequestPurchase(int32 ItemIndex);
	UFUNCTION(Server, Reliable)
		void ServerPurchaseItem(int32 ItemIndex);
	UFUNCTION(Client, Reliable)
		void ClientPurchaseComplete(int32 WeaponSlotIndex);
	UFUNCTION(Client, Reliable)
		void ClientGrantArmorComplete(UArmorData* ArmorDataPtr);
	UFUNCTION(Client, Reliable)
		void ClientGrenadePurchaseComplete();
	void ToggleScoreboard();

	UFUNCTION(BlueprintCallable)
		void SentMessageToServer(const FString& Message, bool bIsTeam);
	UFUNCTION(Server,Reliable)
		void ServerReceivedMessage(const FString& Message, bool bIsTeam);
public:
	UFUNCTION(BlueprintCallable)
		void ToggleBuyMenu();
	UFUNCTION(BlueprintCallable)
		void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<UUserWidget> MainMenuWidgetClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<UHUDWidget> HUDWidgetClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		TObjectPtr<UHUDWidget> HUDWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<UUserWidget> BuyMenuWidgetClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		TObjectPtr<UUserWidget> BuyMenuWidget;
	UFUNCTION(BlueprintCallable)
		void TogglePause();

private:
	UFUNCTION()
		void OnPawnHealthChanged(float Health, float MaxHealth);
	UFUNCTION()
		void OnPawnHealingStateChanged();

	UPROPERTY()
		TObjectPtr<As1mpleFpsCharacter> BoundCharacter;
};
