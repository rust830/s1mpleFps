// Copyright Epic Games, Inc. All Rights Reserved.


#include "s1mpleFpsPlayerController.h"
#include "ChatWidget.h"
#include "HUDWidget.h"
#include "MinimapWidget.h"
#include "s1mpleFpsCharacter.h"
#include "HealthComponent.h"
#include "WeaponInventoryComponent.h"
#include "GrenadeComponent.h"
#include "DamageComponent.h"
#include "EnhancedInputSubsystems.h"
#include "UserSettings/EnhancedInputUserSettings.h"
#include "EnhancedInputComponent.h"
#include "InputActionValue.h"
#include "s1mpleFpsPlayerState.h"
#include "TP_WeaponComponent.h"
#include "Engine/LocalPlayer.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsGameInstance.h"
#include "LobbyGameMode.h"


void As1mpleFpsPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 用主菜单设置的玩家名覆盖登录名（本地玩家才发 RPC）
	if (IsLocalController())
	{
		if (Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>())
		{
			const FString& DesiredName = GI->GetDesiredPlayerName();
			if (!DesiredName.IsEmpty())
			{
				ServerSetPlayerName(DesiredName);
			}
		}
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (InputMappingContext)
		{
			Subsystem->AddMappingContext(InputMappingContext, 0);
			// UE5.3+ 改键系统：把 IMC 注册到 UserSettings，MapPlayerKey 才能找到可映射的动作。
			// 注意：2D 轴动作（Move/Look）不能标 Player Mappable，否则轴向修饰器会丢失、WASD 全变同一方向。
			if (UEnhancedInputUserSettings* UserSettings = Subsystem->GetUserSettings())
			{
				UserSettings->RegisterInputMappingContext(InputMappingContext);
			}
		}
	}

	if (!IsLocalController())
	{
		return;
	}

	FString MapName = GetWorld()->GetMapName();
	const bool bIsMenuMap = MapName.Contains(TEXT("Start")) || MapName.Contains(TEXT("MainMenu"));
	const bool bIsLobbyMap = MapName.Contains(TEXT("Lobby"));

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
	else if (bIsLobbyMap && LobbyWidgetClass)
	{
		// 大厅地图：创建大厅 UI（选队/就绪/开始）
		LobbyWidget = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
		if (LobbyWidget)
		{
			LobbyWidget->AddToViewport();
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
		// 创建小地图（刷新玩家/占点点位图标）
		if (MinimapWidgetClass)
		{
			MinimapWidget = CreateWidget<UMinimapWidget>(this, MinimapWidgetClass);
			if (MinimapWidget)
			{
				MinimapWidget->AddToViewport();
			}
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
		if (MapAction)
		{
			Input->BindAction(MapAction, ETriggerEvent::Started, this, &As1mpleFpsPlayerController::ToggleBigMap);
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

void As1mpleFpsPlayerController::ToggleBigMap()
{
	if (!BigMapWidget && BigMapWidgetClass)
	{
		BigMapWidget = CreateWidget<UMinimapWidget>(this, BigMapWidgetClass);
		if (BigMapWidget)
		{
			BigMapWidget->AddToViewport();
			BigMapWidget->SetVisibility(ESlateVisibility::Collapsed); // 默认隐藏，按 Tab 才显示
		}
	}

	if (BigMapWidget)
	{
		const bool bVisible = BigMapWidget->GetVisibility() == ESlateVisibility::Visible;
		BigMapWidget->SetVisibility(bVisible ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	}
}

void As1mpleFpsPlayerController::OpenChatBox(bool bIsTeam)
{
	if (bChatOpen) return;
	bChatOpen = true;
	bCurrentChatIsTeam = bIsTeam;
	
	if (ChatClass && !ChatWidget) {
		ChatWidget = CreateWidget<UChatWidget>(this, ChatClass);
		
	}
	if (ChatWidget) {
		ChatWidget->SetOwningPC(this);
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
		// 数据驱动：GrantWeapon 内部 SyncWeaponSlots() 会把武器数据复制给客户端，客户端本地建枪，无需额外 RPC
		PC->WeaponInventoryComponent->GrantWeapon(EffectiveClass, Item.WeaponData);
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
		BoundCharacter->HealthComponent->OnHealthChanged.RemoveDynamic(this, &As1mpleFpsPlayerController::OnPawnHealthChanged);
		BoundCharacter = nullptr;
	}

	if (As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(InPawn))
	{
		

		Char->HealthComponent->OnHealthChanged.AddDynamic(this, &As1mpleFpsPlayerController::OnPawnHealthChanged);
		Char->HealthComponent->OnHealingStateChanged.AddDynamic(this, &As1mpleFpsPlayerController::OnPawnHealingStateChanged);
		BoundCharacter = Char;
		Char->HealthComponent->OnRep_Health();

		// Seed weapon/armor name + ammo display on initial spawn.
		// Without this, the text stays hidden until the first SwitchWeapon call.
		if (HUDWidget)
		{
			if (Char->WeaponInventoryComponent->CurrentWeapon)
			{
				
				HUDWidget->BindToWeapon(Char->WeaponInventoryComponent->CurrentWeapon);
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
		BoundCharacter->HealthComponent->OnHealthChanged.RemoveDynamic(this, &As1mpleFpsPlayerController::OnPawnHealthChanged);
		BoundCharacter->HealthComponent->OnHealingStateChanged.RemoveDynamic(this, &As1mpleFpsPlayerController::OnPawnHealingStateChanged);
		BoundCharacter = nullptr;
	}

	Super::OnUnPossess();
}

void As1mpleFpsPlayerController::ServerHostStartGame_Implementation()
{
	ALobbyGameMode* GM = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
	if (GM && GM->IsHost(this))
	{
		GM->TryStartGame();
	}
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
		HUDWidget->UpdateHealingDisplay(Char->HealthComponent->bIsHealing, Char->HealthComponent->HealingDuration);
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

void As1mpleFpsPlayerController::ServerJoinTeam_Implementation(ETeam NewTeam)
{
	if (NewTeam == ETeam::None)return;
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (PS) {
		PS->Team = NewTeam;
		PS->ForceNetUpdate();
		// 跨地图携带队伍（大厅→PvP）
		if (Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>()) {
			GI->SetPlayerTeam(PS->GetUniqueId(), NewTeam);
		}
		ALobbyGameMode* GM = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
		if(GM)
		GM->CheckStartCondition();
	}
}

void As1mpleFpsPlayerController::ServerSetReady_Implementation()
{
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS)return;
	PS->bReady = !PS->bReady;
	PS->ForceNetUpdate();
	ALobbyGameMode* GM = GetWorld()->GetAuthGameMode<ALobbyGameMode>();
	if (GM)
		GM->CheckStartCondition();
}

void As1mpleFpsPlayerController::ServerSetPlayerName_Implementation(const FString& NewName)
{
	if (NewName.IsEmpty()) return;
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (PS)
	{
		PS->SetPlayerName(NewName);
	}
}

void As1mpleFpsPlayerController::ServerSelectHero_Implementation(int32 HeroIndex)
{
	if (HeroIndex < 0 || HeroIndex >= HeroRoster.Num()) return; // 越界保护
	As1mpleFpsPlayerState* PS = GetPlayerState<As1mpleFpsPlayerState>();
	if (!PS) return;

	PS->SelectedHeroIndex = HeroIndex;
	PS->ForceNetUpdate();
	// 跨地图携带（大厅 → PvP），和队伍携带同理
	if (Us1mpleFpsGameInstance* GI = GetGameInstance<Us1mpleFpsGameInstance>())
	{
		GI->SetPlayerHero(PS->GetUniqueId(), HeroIndex);
	}
	// 服务器端立即解析并换模型（写入 Character 的复制字段 → 客户端 OnRep_HeroVisual 自动同步）
	if (As1mpleFpsCharacter* C = Cast<As1mpleFpsCharacter>(GetPawn()))
	{
		C->ApplyHeroVisual();
	}
	UE_LOG(LogTemp, Log, TEXT("[Hero] ServerSelectHero: hero=%d, Player=%s"), HeroIndex, *PS->GetPlayerName());
}

void As1mpleFpsPlayerController::ClientUpdatePrompt_Implementation(bool bShow, const FString& Text)
{
	if (!IsLocalController())return;

	if (HUDWidget)
	{
		HUDWidget->SetInteractionPrompt(bShow, Text);
	}
}

UHeroData* As1mpleFpsPlayerController::GetHeroByIndex(int32 Index) const
{
	return HeroRoster.IsValidIndex(Index) ? HeroRoster[Index] : nullptr;
}
