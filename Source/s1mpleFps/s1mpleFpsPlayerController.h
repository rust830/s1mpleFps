// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BuyEquipmentData.h"
#include "s1mpleFpsPlayerState.h"
#include "HeroData.h"
#include "s1mpleFpsPlayerController.generated.h"

class UChatWidget;



class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UHUDWidget;
class UMinimapWidget;
class UBuyEquipmentData;
class As1mpleFpsCharacter;
class ADoor;


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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input)
	UInputAction* MapAction;

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
		void ToggleBigMap();
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
		TSubclassOf<UMinimapWidget> MinimapWidgetClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		TObjectPtr<UMinimapWidget> MinimapWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<UMinimapWidget> BigMapWidgetClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		TObjectPtr<UMinimapWidget> BigMapWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<UUserWidget> BuyMenuWidgetClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		TObjectPtr<UUserWidget> BuyMenuWidget;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
		TSubclassOf<UUserWidget> LobbyWidgetClass;
	UPROPERTY(BlueprintReadOnly, Category = "UI")
		TObjectPtr<UUserWidget> LobbyWidget;
	UFUNCTION(BlueprintCallable)
		void TogglePause();

	// 门交互：门不归客户端拥有，客户端直接在门上发 Server RPC 会被丢弃（No owning connection）；
	// 改在 PC 上发（客户端拥有自己的 PC）。服务器实现里再调用 Door->RequestOpen 完成开门。
	UFUNCTION(Server, Reliable)
		void ServerInteractDoor(ADoor* Door);

	//lobby RPC
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerJoinTeam(ETeam NewTeam);
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetReady();
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerHostStartGame();
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSetPlayerName(const FString& NewName);
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerSelectHero(int32 HeroIndex);
	UFUNCTION(Client,Reliable)
	void ClientUpdatePrompt(bool bShow, const FString& Text);

	// 英雄/皮肤列表（选人 UI + 角色换模型共用的唯一配置源，在 BP_FirstPersonPlayerController 里配）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heroes")
	TArray<UHeroData*> HeroRoster;

	UFUNCTION(BlueprintCallable, Category = "Heroes")
	UHeroData* GetHeroByIndex(int32 Index) const;

private:
	UFUNCTION()
		void OnPawnHealthChanged(float Health, float MaxHealth);
	UFUNCTION()
		void OnPawnHealingStateChanged();

	UPROPERTY()
		TObjectPtr<As1mpleFpsCharacter> BoundCharacter;
};
