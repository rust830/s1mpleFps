// Copyright Epic Games, Inc. All Rights Reserved.


#include "s1mpleFpsPlayerController.h"
#include "ChatWidget.h"
#include "HUDWidget.h"
#include "s1mpleFpsCharacter.h"
#include "GrenadeComponent.h"
#include "DamageComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "s1mpleFpsPlayerState.h"
#include "TP_WeaponComponent.h"
#include "Engine/LocalPlayer.h"
#include "s1mpleFpsGameState.h"

void As1mpleFpsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
		}
		else
		{
			
		}
	}

	if (!IsLocalController())
	{
		return;
	}

	FString MapName = GetWorld()->GetMapName();
	const bool bIsMenuMap = MapName.Contains(TEXT("Start")) || MapName.Contains(TEXT("MainMenu"));

	if (bIsMenuMap)
	{
		// 菜单地图：创建主菜单 UI（服务端和客户端各自执行）
		if (MainMenuWidgetClass)
		{
			
			UUserWidget* Menu = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
			if (Menu)
			{
				Menu->AddToViewport();
			}
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
	else if (HUDWidgetClass)
	{
		// 战斗地图：创建 HUD
		
		HUDWidget = CreateWidget<UHUDWidget>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			
		}
		else
		{
			
		}
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
	}
	else
	{
		
	}
}

void As1mpleFpsPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (ScoreboardAction)
		{
			Input->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &As1mpleFpsPlayerController::ToggleScoreboard);
		}
		if (BuyMenuAction) {
			Input->BindAction(BuyMenuAction, ETriggerEvent::Started, this, &As1mpleFpsPlayerController::ToggleBuyMenu);
		}
		if (OpenTeamChat) {
			Input->BindAction(OpenTeamChat, ETriggerEvent::Started, this, &As1mpleFpsPlayerController::OpenChatBox,true);
		}
		if (OpenAllChat) {
			Input->BindAction(OpenAllChat, ETriggerEvent::Started, this, &As1mpleFpsPlayerController::OpenChatBox,false);
			}
			if (EscapeAction) {
				Input->BindAction(EscapeAction, ETriggerEvent::Started, this, &As1mpleFpsPlayerController::OnEscapePressed);
		}
	}

}

void As1mpleFpsPlayerController::OpenChatBox(bool bIsTeam)
{
	if (bChatOpen)return;
	bChatOpen = true;
	bCurrentChatIsTeam = bIsTeam;
	
	if (ChatClass && !ChatWidget) {
		ChatWidget = CreateWidget<UChatWidget>(this, ChatClass);
		
	}
	if (ChatWidget) {
		ChatWidget->AddToViewport();
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void As1mpleFpsPlayerController::CloseChatBox()
{
	
	bChatOpen = false;
	if (ChatWidget) {
		ChatWidget->RemoveFromParent();
		ChatWidget = nullptr;
	}
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
}

void As1mpleFpsPlayerController::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	
	if (CommitMethod == ETextCommit::Type::OnEnter)
	{
		FString Message = Text.ToString();
		if (!Message.IsEmpty())
		{
			SentMessageToServer(Message, bCurrentChatIsTeam);
			return;
		}
	}
	CloseChatBox();
}

void As1mpleFpsPlayerController::OnEscapePressed()
{
	if (bChatOpen)
	{
		CloseChatBox();
		return;
	}
	TogglePause();
}

const TArray<FBuyItemEntry>& As1mpleFpsPlayerController::GetBuyItems() const
{
	static const TArray<FBuyItemEntry> Empty;
	return BuyEquipmentData ? BuyEquipmentData->Items : Empty;
}

int32 As1mpleFpsPlayerController::GetCurrentMoney() const
{
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS)return 0;
	return PS->Money;
	
}

void As1mpleFpsPlayerController::RequestPurchase(int32 ItemIndex)
{
	if (!BuyEquipmentData || !BuyEquipmentData->Items.IsValidIndex(ItemIndex))return;
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS || PS->Money < BuyEquipmentData->Items[ItemIndex].Price)return;
	ServerPurchaseItem(ItemIndex);
}

void As1mpleFpsPlayerController::ServerPurchaseItem_Implementation(int32 ItemIndex)
{
	if (!BuyEquipmentData || !BuyEquipmentData->Items.IsValidIndex(ItemIndex))return;
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS)return;
	FBuyItemEntry Item = BuyEquipmentData->Items[ItemIndex];
	As1mpleFpsCharacter* PC = Cast<As1mpleFpsCharacter>(GetPawn());
	if (!PC)return;
	if (!PS->TrySpendingMoney(Item.Price))return;
	if (Item.WeaponData)
	{
		TSubclassOf<UTP_WeaponComponent> EffectiveClass = Item.WeaponClass;
		if (!EffectiveClass)
			EffectiveClass = UTP_WeaponComponent::StaticClass();
		int32 SlotIndex = PC->GrantWeapon(EffectiveClass, Item.WeaponData);
		if (SlotIndex >= 0)
			ClientPurchaseComplete(SlotIndex);
	}
	else if (Item.ArmorData)
	{
		PC->GrantArmor(Item.ArmorData);
		ClientGrantArmorComplete(Item.ArmorData);
	}
	else if (Item.HealthData)
	{
		PC->GrantHealthItem(Item.HealthData);
	}
	else if (Item.GrenadeData)
	{
		UGrenadeComponent* GC = PC->FindComponentByClass<UGrenadeComponent>();
		if (!GC)
		{
			
			return;
		}
		GC->AddGrenade(Item.GrenadeData, Item.GrenadeAmount);
		ClientGrenadePurchaseComplete();
	}
}

void As1mpleFpsPlayerController::ClientPurchaseComplete_Implementation(int32 WeaponSlotIndex)
{
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(GetPawn());
	if (!Char) return;

	

	if (Char->WeaponInventory.IsValidIndex(WeaponSlotIndex) && Char->WeaponInventory[WeaponSlotIndex])
	{
		UTP_WeaponComponent* Weapon = Char->WeaponInventory[WeaponSlotIndex];
		Weapon->SetOwningCharacter(Char);
		Char->CurrentWeapon = nullptr;
		Char->SwitchWeapon(WeaponSlotIndex);
	}
	else
	{
		
		Char->PendingPurchaseIndex = WeaponSlotIndex;
	}
}

void As1mpleFpsPlayerController::ClientGrantArmorComplete_Implementation(UArmorData* ArmorDataPtr)
{
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(GetPawn());
	if (!Char || !ArmorDataPtr) return;
	if (!Char->DamageComponent) return;

	Char->DamageComponent->EquippedArmors.AddUnique(ArmorDataPtr);
	if (HUDWidget)
	{
		HUDWidget->UpdateEquipmentDisplay();
	}
}

void As1mpleFpsPlayerController::ClientGrenadePurchaseComplete_Implementation()
{
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(GetPawn());
	if (!Char) return;
	UGrenadeComponent* GC = Char->FindComponentByClass<UGrenadeComponent>();
	if (!GC) return;
	GC->OnGrenadeInventoryChanged.Broadcast();
}

void As1mpleFpsPlayerController::ToggleScoreboard()
{
	if (HUDWidget)
	{
		HUDWidget->ToggleScoreboard();
	}
}

void As1mpleFpsPlayerController::SentMessageToServer(const FString& Message, bool bIsTeam)
{
	
	if (Message.IsEmpty())return;
	CloseChatBox();
	ServerReceivedMessage(Message, bIsTeam);
}

void As1mpleFpsPlayerController::ServerReceivedMessage_Implementation(const FString& Message, bool bIsTeam)
{
	
	if (Message.IsEmpty() || Message.Len() > 200) return;
	const FString Sender = GetPlayerState<APlayerState>() ? GetPlayerState<APlayerState>()->GetPlayerName() : TEXT("Unknown");
	
	As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>();
	if (!GS) {  return; }
	
	GS->MulticastReceivedChatMessage(Sender, Message, bIsTeam);
}

void As1mpleFpsPlayerController::ToggleBuyMenu()
{
	if (BuyMenuWidget) {
		BuyMenuWidget->RemoveFromParent();
		BuyMenuWidget = nullptr;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
		return;
	}
	if (!BuyMenuWidgetClass )return;
	BuyMenuWidget = CreateWidget<UUserWidget>(this, BuyMenuWidgetClass);
	if (BuyMenuWidget) {
		BuyMenuWidget->AddToViewport();
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void As1mpleFpsPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BoundCharacter)
	{
		BoundCharacter->OnHealthChanged.RemoveDynamic(this, &As1mpleFpsPlayerController::OnPawnHealthChanged);
		BoundCharacter = nullptr;
	}

	if (As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(InPawn))
	{
		

		Char->OnHealthChanged.AddDynamic(this, &As1mpleFpsPlayerController::OnPawnHealthChanged);
		Char->OnHealingStateChanged.AddDynamic(this, &As1mpleFpsPlayerController::OnPawnHealingStateChanged);
		BoundCharacter = Char;
		Char->OnRep_Health();

		// Seed weapon/armor name + ammo display on initial spawn.
		// Without this, the text stays hidden until the first SwitchWeapon call.
		if (HUDWidget)
		{
			if (Char->CurrentWeapon)
			{
				
				HUDWidget->BindToWeapon(Char->CurrentWeapon);
			}
			else
			{
				
				HUDWidget->UpdateEquipmentDisplay();
			}
		}
	}
}

void As1mpleFpsPlayerController::OnUnPossess()
{
	if (BoundCharacter)
	{
		BoundCharacter->OnHealthChanged.RemoveDynamic(this, &As1mpleFpsPlayerController::OnPawnHealthChanged);
		BoundCharacter->OnHealingStateChanged.RemoveDynamic(this, &As1mpleFpsPlayerController::OnPawnHealingStateChanged);
		BoundCharacter = nullptr;
	}

	Super::OnUnPossess();
}

void As1mpleFpsPlayerController::OnPawnHealthChanged(float Health, float MaxHealth)
{
	
	if (HUDWidget)
	{
		HUDWidget->UpdateHealthDisplay(Health, MaxHealth);
	}
}

void As1mpleFpsPlayerController::OnPawnHealingStateChanged()
{
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(GetPawn());
	if (!Char) return;
	if (HUDWidget)
	{
		HUDWidget->UpdateHealingDisplay(Char->bIsHealing, Char->HealingDuration);
	}
}

void As1mpleFpsPlayerController::TogglePause()
{
	if (!PauseMenuClass) {
		
		return;
	}
	if (IsPaused()) {
		if (PauseMenuWidget) {
			PauseMenuWidget->RemoveFromParent();
			PauseMenuWidget = nullptr;
		}
		FInputModeGameOnly InputGame;
		SetInputMode(InputGame);
		bShowMouseCursor = false;

		SetPause(false);
	}
	else {
		SetPause(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		PauseMenuWidget = CreateWidget<UUserWidget>(this, PauseMenuClass);
		if (PauseMenuWidget) {
			PauseMenuWidget->AddToViewport();
		}
		else {
			
		}

	}

}
