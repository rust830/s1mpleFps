#include "HUDWidget.h"
#include "TP_WeaponComponent.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "s1mpleFpsCharacter.h"
#include "HealthComponent.h"
#include "HealthData.h"
#include "WeaponInventoryComponent.h"
#include "DamageComponent.h"
#include "ArmorData.h"
#include "WeaponData.h"
#include "GrenadeComponent.h"
#include "GrenadeData.h"
#include "GrenadeSlotEntry.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "s1mpleFpsGameInstance.h"
#include "GameFramework/PlayerState.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"  
#include "Engine/Font.h"        


struct FKillPlayParams {
	FString KillerName;
	FString VictimName;
	ETeam KillerTeam;   // 击杀者队伍：蓝图据此做不同队伍的不同播报效果
	FKillPlayParams(const FString& InKillerName, const FString& InVictimName, ETeam InKillerTeam)
		: KillerName(InKillerName), VictimName(InVictimName), KillerTeam(InKillerTeam) {
	}
};

void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	
	CustomFont = LoadObject<UFont>(nullptr, TEXT("/Game/FirstPerson/Blueprints/Doors/Cinzel-VariableFont_wght_Font.Cinzel-VariableFont_wght_Font"));
	if (CustomFont)
	{
		CustomFontInfo = CustomFont->GetLegacySlateFontInfo(); // 无参数获取字体信息
		CustomFontInfo.Size = 14;                             // 手动设置默认大小
	}
	else
	{
		// 加载失败时回退到引擎默认字体
		CustomFontInfo = FCoreStyle::GetDefaultFontStyle("Regular", 14);
		UE_LOG(LogTemp, Warning, TEXT("Custom font not found, using default."));
	}

	HitMarkerAlpha = 0.0f;
	HitMarkerDuration = 0.0f;
	bHitMarkerIsEnemy = false;
	//硬编码颜色
	HealthGreen = FLinearColor(0.08f, 0.01f, 0.005f);
	HealthYellow = FLinearColor(0.25f, 0.04f, 0.02f);
	HealthRed = FLinearColor(0.35f, 0.05f, 0.03f);

	// Default text values - prevents "Text Block" placeholder
	if (AmmoText) AmmoText->SetText(FText::FromString(TEXT("0 / 0")));
	if (HealthText) HealthText->SetText(FText::AsNumber(100));
	if (HealthPercentText) HealthPercentText->SetText(FText::FromString(TEXT("100%")));
	if (HealthBar) { HealthBar->SetPercent(1.0f); HealthBar->SetFillColorAndOpacity(HealthGreen); }
	if (KillsText) KillsText->SetText(FText::AsNumber(0));
	if (DeathsText) DeathsText->SetText(FText::AsNumber(0));
	if (ScoreText) ScoreText->SetText(FText::AsNumber(0));
	if (MatchTimeText) MatchTimeText->SetText(FText::FromString(TEXT("10:00")));
	if (WeaponNameText) WeaponNameText->SetText(FText::FromString(TEXT("")));
	if (ArmorNameText) ArmorNameText->SetText(FText::FromString(TEXT("")));

	// 热身倒计时
	if (WarmUpCountdownText)
	{
		FSlateFontInfo WarmUpFont = WarmUpCountdownText->GetFont();
		if (WarmUpFont.Size <= 0) WarmUpFont.Size = WarmUpFontSize;
		WarmUpCountdownText->SetFont(WarmUpFont);
		WarmUpCountdownText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Hide hit marker, scoreboard, and ammo initially
	if (HitMarkerImage) { HitMarkerImage->SetOpacity(0.0f); HitMarkerImage->SetVisibility(ESlateVisibility::Hidden); }
	if (ScoreboardPanel) ScoreboardPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (AmmoText) AmmoText->SetVisibility(ESlateVisibility::Hidden);
	if (HealingText) HealingText->SetVisibility(ESlateVisibility::Hidden);
	if (MedicineIconImage) MedicineIconImage->SetVisibility(ESlateVisibility::Hidden);
	if (MedicineCountText) MedicineCountText->SetVisibility(ESlateVisibility::Hidden);
	if (HealingRingImage)
	{
		HealingRingImage->SetVisibility(ESlateVisibility::Hidden);
		
		// HealingRingImage->SetColorAndOpacity(HealingRingColor);
		if (HealingRingBaseMaterial)
		{
			HealingRingMaterial = UMaterialInstanceDynamic::Create(HealingRingBaseMaterial, this);
			HealingRingImage->SetBrushFromMaterial(HealingRingMaterial);
			HealingRingMaterial->SetScalarParameterValue(FName("Progress"), 0.0f);
		}
	}

	// Grenade UI — 槽位按背包动态生成（初始清空）；cooking 只在烹饪时显示
	if (GrenadeSlotBox) GrenadeSlotBox->ClearChildren();
	if (CookingTimeText) CookingTimeText->SetVisibility(ESlateVisibility::Hidden);
	if (CookingRingImage)
	{
		CookingRingImage->SetVisibility(ESlateVisibility::Hidden);
		if (CookingRingBaseMaterial)
		{
			CookingRingMaterial = UMaterialInstanceDynamic::Create(CookingRingBaseMaterial, this);
			CookingRingImage->SetBrushFromMaterial(CookingRingMaterial);
			CookingRingMaterial->SetScalarParameterValue(FName("Progress"), 0.0f);
		}
	}

	// Match end buttons
	if (RestartButton) RestartButton->OnClicked.AddDynamic(this, &UHUDWidget::RestartMatch);
	if (LeaveButton) LeaveButton->OnClicked.AddDynamic(this, &UHUDWidget::LeaveMatch);
	if (MatchEndPanel) MatchEndPanel->SetVisibility(ESlateVisibility::Collapsed);


	if (DoorPromptText)DoorPromptText->SetVisibility(ESlateVisibility::Collapsed);
	// Try initial bind - may fail on clients if PlayerState hasn't replicated yet
	TryBindPlayerState();
}

void UHUDWidget::TryBindPlayerState()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	if (!bPlayerStateBound)
	{
		if (As1mpleFpsPlayerState* PS = PC->GetPlayerState<As1mpleFpsPlayerState>())
		{
			PS->OnScoreChanged.AddDynamic(this, &UHUDWidget::UpdateScoreDisplay);
			UpdateScoreDisplay(PS->Kills, PS->Deaths, PS->Scores);
			bPlayerStateBound = true;
		}
	}


	if (!bGameStateBound)
	{
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>())
		{
			GS->OnMatchTimeChanged.AddDynamic(this, &UHUDWidget::UpdateMatchTimeDisplay);
			UpdateMatchTimeDisplay(GS->MatchTimeRemaining);
			bGameStateBound = true;
		}
	}
	if (!bKillPlayBound) {
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>()) {
			GS->OnKillPlay.AddDynamic(this, &UHUDWidget::OnKillPlayReceived);
			bKillPlayBound = true;
		}
	}
	if (!bMatchEndBound) {
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>()) {
			GS->OnMatchEnded.AddDynamic(this, &UHUDWidget::OnMatchEndedReceived);
			bMatchEndBound = true;
		}
	}
	if (!bSuddenDeathBound) {
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>()) {
			GS->OnSuddenDeath.AddDynamic(this, &UHUDWidget::OnSuddenDeathReceived);
			bSuddenDeathBound = true;
		}
	}
	if (!bOvertimeBound) {
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>()) {
			GS->OnOvertimeChanged.AddDynamic(this, &UHUDWidget::OnOvertimeReceived);
			bOvertimeBound = true;
		}
	}
	if (!bWarmUpBound) {
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>()) {
			GS->OnWarmUpTimeChanged.AddDynamic(this, &UHUDWidget::OnWarmUpReceived);
			bWarmUpBound = true;

		}
	}
	if (!bChatMessageBound) {
		if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>()) {
			GS->OnMessageReceived.AddDynamic(this, &UHUDWidget::OnChatMessageReceived);
			bChatMessageBound = true;

		}
	}
	if (!bGrenadeBound)
	{
		BindToGrenadeComponent();
	}
	if (!bHealthBound)
	{
		BindToHealthComponent();
	}
}

void UHUDWidget::RemoveEntryInterval(UUserWidget* Entry)
{
	if (!Entry)return;

	KillPlayBox->RemoveChild(Entry);

	if (EntryTimerMap.Contains(Entry)) {
		GetWorld()->GetTimerManager().ClearTimer(EntryTimerMap[Entry]);
		EntryTimerMap.Remove(Entry);
	}

	Entry->RemoveFromParent();
}

void UHUDWidget::OnEntryTimerElapsed(UUserWidget* Entry)
{
	if (!Entry)return;
	if (Entry->GetParent() == KillPlayBox) {
		RemoveEntryInterval(Entry);
	}
}

void UHUDWidget::OnMatchEndedReceived(const FString& WinnerName, bool bWinByKill)
{

	APlayerController* PC = GetOwningPlayer();
	if (!PC)return;
	if (As1mpleFpsCharacter* Character = Cast<As1mpleFpsCharacter>(PC->GetPawn())) {
		Character->HealthComponent->HideDeathWidget();
	}
	bool bIsWinner = PC && PC->PlayerState && PC->PlayerState->GetPlayerName() == WinnerName;

	if (MatchEndPanel)
	{
		MatchEndPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (MatchEndResultText)
	{
		MatchEndResultText->SetText(FText::FromString(bIsWinner ? TEXT("TRIUMPH") : TEXT("LOST TO BLADE")));
	}

	if (MatchEndInfoText)
	{
		FString Reason = bWinByKill ? TEXT("BLOODSHED DONE") : TEXT("THE FINAL BELL");
		FString Info = FString::Printf(TEXT("WHO PREVAIL %s（%s）"), *WinnerName, *Reason);
		MatchEndInfoText->SetText(FText::FromString(Info));
	}
	PC->bShowMouseCursor = true;
	FInputModeUIOnly InputMode;
	InputMode.SetWidgetToFocus(TakeWidget());
	PC->SetInputMode(InputMode);

}

void UHUDWidget::OnSuddenDeathReceived()
{
	if (MatchTimeText) {
		MatchTimeText->SetColorAndOpacity(HealthRed); // #E63946
	}
}

void UHUDWidget::OnWarmUpReceived(float WarmUpTimeRemaining)
{

	if (!WarmUpCountdownText) return;

	if (WarmUpTimeRemaining > 0.f)
	{
		WarmUpCountdownText->SetVisibility(ESlateVisibility::Visible);
		WarmUpCountdownText->SetText(FText::AsNumber(FMath::CeilToInt32(WarmUpTimeRemaining)));
	}
	else
	{
		WarmUpCountdownText->SetText(FText::FromString(TEXT("GO!")));
		// 0.8 秒后自动隐藏
		GetWorld()->GetTimerManager().ClearTimer(WarmUpHideHandle);
		GetWorld()->GetTimerManager().SetTimer(WarmUpHideHandle, [this]()
			{
				if (WarmUpCountdownText)
				{
					WarmUpCountdownText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}, WarmUpDisplayTime, false);
	}
}

void UHUDWidget::OnOvertimeReceived(float OvertimeRemaining)
{
	if (!MatchTimeText) return;

	int32 Minutes = FMath::FloorToInt32(OvertimeRemaining / 60.0f);
	int32 Seconds = FMath::FloorToInt32(FMath::Fmod(OvertimeRemaining, 60.0f));

	FText TimeText = (Seconds < 10)
		? FText::Format(FText::FromString(TEXT("NOW OR NEVER LAST CHANCE TO TURN THE TIDE\n {0}:0{1}")), FText::AsNumber(Minutes), FText::AsNumber(Seconds))
		: FText::Format(FText::FromString(TEXT("NOW OR NEVER LAST CHANCE TO TURN THE TIDE\n {0}:{1}")), FText::AsNumber(Minutes), FText::AsNumber(Seconds));

	MatchTimeText->SetText(TimeText);

	if (OvertimeTickSound)
	{
		UGameplayStatics::PlaySound2D(this, OvertimeTickSound);
	}
}

void UHUDWidget::RestartMatch()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC && PC->HasAuthority())
	{
		GetWorld()->ServerTravel(TEXT("?Restart"));
	}
}

void UHUDWidget::LeaveMatch()
{
	// 短名 → 全路径（打包后 OpenLevel 不认短名，会回退到默认地图）
	UGameplayStatics::OpenLevel(this, FName(*Us1mpleFpsGameInstance::ResolveMapPath(TEXT("StartMap"))));
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Retry binding if PlayerState/GameState wasn't ready during NativeConstruct
	if (!bPlayerStateBound || !bGameStateBound || !bKillPlayBound || !bMatchEndBound || !bSuddenDeathBound || !bOvertimeBound || !bWarmUpBound || !bChatMessageBound || !bGrenadeBound || !bHealthBound)
	{
		TryBindPlayerState();
	}

	// Hit marker fade
	if (HitMarkerDuration > 0.0f)
	{
		HitMarkerDuration -= InDeltaTime;
		if (HitMarkerDuration <= 0.0f)
		{
			HitMarkerAlpha = 0.0f;
			HitMarkerDuration = 0.0f;
		}
		else
		{
			HitMarkerAlpha = FMath::Max(0.0f, HitMarkerDuration / 0.3f);
		}
		if (HitMarkerImage)
		{
			HitMarkerImage->SetOpacity(HitMarkerAlpha);
			HitMarkerImage->SetColorAndOpacity(bHitMarkerIsEnemy ? HealthRed : FLinearColor::White);
			if (HitMarkerAlpha <= 0.0f)
			{
				HitMarkerImage->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}

	// Healing countdown
	if (bIsHealingActive)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		float Remaining = FMath::Max(0.0f, HealingEndTime - CurrentTime);
		float Progress = HealingTotalDuration > 0.0f
			? FMath::Clamp(Remaining / HealingTotalDuration, 0.0f, 1.0f)
			: 0.0f;

		if (HealingRingMaterial)
			HealingRingMaterial->SetScalarParameterValue(FName("Progress"), Progress);

		if (HealingText)
		{
			if (Remaining > 0.0f)
			{
				HealingText->SetText(FText::AsNumber(FMath::CeilToInt32(Remaining)));
			}
			else
			{
				bIsHealingActive = false;
				HealingText->SetVisibility(ESlateVisibility::Hidden);
			}
		}
	}
}

void UHUDWidget::OnChatMessageReceived(const FString& Sender, const FString& Message, bool bIsTeam)
{

	if (!ChatMessageBox) return;
	if (Sender.IsEmpty() || Message.IsEmpty()) return;

	// Remove oldest if at max
	while (ChatMessageBox->GetChildrenCount() >= MaxChatMessages)
	{
		if (UTextBlock* Oldest = Cast<UTextBlock>(ChatMessageBox->GetChildAt(0)))
		{
			if (ChatEntryTimerMap.Contains(Oldest))
			{
				GetWorld()->GetTimerManager().ClearTimer(ChatEntryTimerMap[Oldest]);
				ChatEntryTimerMap.Remove(Oldest);
			}
		}
		ChatMessageBox->RemoveChildAt(0);
	}

	const FString Prefix = bIsTeam ? TEXT("[Team] ") : TEXT("");
	const FString FullText = Prefix + Sender + TEXT(": ") + Message;
	const FLinearColor TeamChatBlue = FLinearColor(0.133f, 0.452f, 1.0f); // #66B2FF
	const FLinearColor Color = bIsTeam ? TeamChatBlue : FLinearColor::White;

	UTextBlock* TextEntry = NewObject<UTextBlock>(this);
	TextEntry->SetText(FText::FromString(FullText));
	TextEntry->SetColorAndOpacity(FSlateColor(Color));

	
	FSlateFontInfo Font = CustomFontInfo;               // 从缓存的字体信息复制
	Font.Size = 14;                                     // 保持原有大小
	Font.OutlineSettings.OutlineSize = 1;
	Font.OutlineSettings.OutlineColor = FLinearColor::Black;
	TextEntry->SetFont(Font);

	TextEntry->SetShadowOffset(FVector2D(1.0f, 1.0f));
	TextEntry->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));

	ChatMessageBox->AddChildToVerticalBox(TextEntry);

	FTimerHandle RemoveHandle;
	GetWorld()->GetTimerManager().SetTimer(RemoveHandle, [this, TextEntry]()
		{
			if (ChatMessageBox && TextEntry)
			{
				ChatMessageBox->RemoveChild(TextEntry);
			}
			ChatEntryTimerMap.Remove(TextEntry);
		}, ChatMessageLifetime, false);
	ChatEntryTimerMap.Add(TextEntry, RemoveHandle);
}

void UHUDWidget::NativeDestruct()
{
	for (auto& Pair : EntryTimerMap) {
		if (GetWorld()) {
			GetWorld()->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	EntryTimerMap.Empty();
	for (auto& ChatPair : ChatEntryTimerMap)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(ChatPair.Value);
		}
	}
	ChatEntryTimerMap.Empty();
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(WarmUpHideHandle);
	}
	Super::NativeDestruct();
}

void UHUDWidget::BindToWeapon(UTP_WeaponComponent* Weapon)
{
	if (!Weapon) return;
	// Guard against duplicate binds — OnRep_CurrentWeapon and SwitchWeapon
	// may both call BindToWeapon for the same weapon during initial spawn.
	Weapon->OnAmmoChanged.RemoveDynamic(this, &UHUDWidget::UpdateAmmoDisplay);
	Weapon->OnAmmoChanged.AddDynamic(this, &UHUDWidget::UpdateAmmoDisplay);
	UpdateAmmoDisplay(Weapon->CurrentAmmo, Weapon->SpareAmmo);
	if (AmmoText) AmmoText->SetVisibility(ESlateVisibility::Visible);
	if (WeaponNameText) WeaponNameText->SetVisibility(ESlateVisibility::Visible);
	if (ArmorNameText) ArmorNameText->SetVisibility(ESlateVisibility::Visible);
	UpdateEquipmentDisplay();
}

void UHUDWidget::PlayHitMarker(bool bIsEnemy)
{
	bHitMarkerIsEnemy = bIsEnemy;
	HitMarkerAlpha = 1.0f;
	HitMarkerDuration = 0.3f;
	if (HitMarkerImage)
	{
		HitMarkerImage->SetVisibility(ESlateVisibility::Visible);
		HitMarkerImage->SetOpacity(1.0f);
		HitMarkerImage->SetColorAndOpacity(bIsEnemy ? HealthRed : FLinearColor::White);
	}
}

void UHUDWidget::UpdateHealthDisplay(float CurrentHealth, float MaxHealth)
{

	float Percent = MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;

	if (HealthBar)
	{
		HealthBar->SetPercent(Percent);
		if (Percent > 0.5f)
			HealthBar->SetFillColorAndOpacity(HealthGreen);
		else if (Percent > 0.25f)
			HealthBar->SetFillColorAndOpacity(HealthYellow);
		else
			HealthBar->SetFillColorAndOpacity(HealthRed);
	}

	if (HealthText)
		HealthText->SetText(FText::AsNumber(FMath::CeilToInt32(CurrentHealth)));

	if (HealthPercentText)
		HealthPercentText->SetText(FText::Format(FText::FromString(TEXT("{0}%")), FText::AsNumber(FMath::CeilToInt32(Percent * 100.0f))));
}

void UHUDWidget::UpdateAmmoDisplay(int32 CurrentAmmo, int32 SpareAmmo)
{

	if (AmmoText)
		AmmoText->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")),
			FText::AsNumber(CurrentAmmo), FText::AsNumber(SpareAmmo)));
}

void UHUDWidget::UpdateHealingDisplay(bool bIsHealing, float Duration)
{
	if (bIsHealing && Duration > 0.0f)
	{
		bIsHealingActive = true;
		HealingEndTime = GetWorld()->GetTimeSeconds() + Duration;
		HealingTotalDuration = Duration;

		if (HealingRingImage)
		{
			HealingRingImage->SetVisibility(ESlateVisibility::Visible);
			if (HealingRingMaterial)
				HealingRingMaterial->SetScalarParameterValue(FName("Progress"), 1.0f);
		}
		if (HealingText)
		{
			HealingText->SetVisibility(ESlateVisibility::Visible);
			HealingText->SetText(FText::AsNumber(FMath::CeilToInt32(Duration)));
		}
	}
	else
	{
		bIsHealingActive = false;
		if (HealingRingImage)
		{
			HealingRingImage->SetVisibility(ESlateVisibility::Hidden);
			if (HealingRingMaterial)
				HealingRingMaterial->SetScalarParameterValue(FName("Progress"), 0.0f);
		}
		if (HealingText)
		{
			HealingText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UHUDWidget::UpdateEquipmentDisplay()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char) return;



	// Weapon name
	if (WeaponNameText)
	{
		WeaponNameText->SetVisibility(ESlateVisibility::Visible);
		if (Char->WeaponInventoryComponent->CurrentWeapon && Char->WeaponInventoryComponent->CurrentWeapon->WeaponData)
		{
			const FString& Name = Char->WeaponInventoryComponent->CurrentWeapon->WeaponData->WeaponName;
			// Fall back to asset name if WeaponName field is empty (unfilled data asset).
			WeaponNameText->SetText(FText::FromString(Name.IsEmpty() ? Char->WeaponInventoryComponent->CurrentWeapon->WeaponData->GetName() : Name));
		}
		else if (Char->WeaponInventoryComponent->CurrentWeapon && !Char->WeaponInventoryComponent->CurrentWeapon->WeaponData)
		{
			// WeaponData didn't replicate — show something so we know the widget works.
			WeaponNameText->SetText(FText::FromString(TEXT("(no data)")));
		}
		else
		{
			WeaponNameText->SetText(FText::FromString(TEXT("")));
		}
	}

	// Armor name
	if (ArmorNameText)
	{
		FString ArmorStr;
		if (Char->DamageComponent)
		{
			for (int32 i = 0; i < Char->DamageComponent->EquippedArmors.Num(); i++)
			{
				if (UArmorData* Armor = Char->DamageComponent->EquippedArmors[i])
				{
					if (!ArmorStr.IsEmpty()) ArmorStr += TEXT(", ");
					ArmorStr += Armor->ArmorName.IsEmpty() ? Armor->GetName() : Armor->ArmorName;
				}
			}
		}
		if (ArmorStr.IsEmpty())
		{
			ArmorNameText->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			ArmorNameText->SetVisibility(ESlateVisibility::Visible);
			ArmorNameText->SetText(FText::FromString(ArmorStr));
		}
	}
}

void UHUDWidget::BindToGrenadeComponent()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char) return;

	UGrenadeComponent* GC = Char->FindComponentByClass<UGrenadeComponent>();
	if (!GC) return;

	GC->OnGrenadeEquipped.AddDynamic(this, &UHUDWidget::OnGrenadeEquippedChanged);
	GC->OnGrenadeInventoryChanged.AddDynamic(this, &UHUDWidget::OnGrenadeInventoryChanged);
	GC->OnCookingStart.AddDynamic(this, &UHUDWidget::OnGrenadeCookingStarted);
	GC->OnCookingProgressChanged.AddDynamic(this, &UHUDWidget::OnGrenadeCookingProgress);

	// Initial refresh — icon + count
	OnGrenadeInventoryChanged();

	bGrenadeBound = true;

}

void UHUDWidget::OnGrenadeEquippedChanged()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char) return;
	UGrenadeComponent* GC = Char->FindComponentByClass<UGrenadeComponent>();
	if (!GC) return;

	// 收回手雷时隐藏 cooking UI
	if (!GC->bIsEquipped)
	{
		if (CookingTimeText)  CookingTimeText->SetVisibility(ESlateVisibility::Hidden);
		if (CookingRingImage)
		{
			CookingRingImage->SetVisibility(ESlateVisibility::Hidden);
			if (CookingRingMaterial)
				CookingRingMaterial->SetScalarParameterValue(FName("Progress"), 0.0f);
		}
	}
}

void UHUDWidget::OnGrenadeInventoryChanged()
{
	// 背包增删（捡雷/扔完）与选中切换（Next/Prev）都会广播这里，统一重建槽位列表
	RefreshGrenadeSlots();
}

void UHUDWidget::BindToHealthComponent()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char || !Char->HealthComponent) return;

	Char->HealthComponent->OnHealthItemsChanged.AddDynamic(this, &UHUDWidget::OnHealthItemsChanged);

	// 初始刷新
	OnHealthItemsChanged();

	bHealthBound = true;
}

void UHUDWidget::OnHealthItemsChanged()
{
	RefreshMedicineDisplay();
}

void UHUDWidget::RefreshMedicineDisplay()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char || !Char->HealthComponent) return;

	UHealthComponent* HC = Char->HealthComponent;
	const bool bHas = HC->HealthTypes.IsValidIndex(0) && HC->HealthAmount.IsValidIndex(0) && HC->HealthAmount[0] > 0;

	if (MedicineIconImage)
	{
		if (bHas && HC->HealthTypes[0] && HC->HealthTypes[0]->Icon)
		{
			MedicineIconImage->SetBrushFromTexture(HC->HealthTypes[0]->Icon);
			MedicineIconImage->SetDesiredSizeOverride(MedicineIconSize);
			// 防止 CanvasPanel 里图标被贴图原始分辨率撑大
			if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MedicineIconImage->Slot))
			{
				CanvasSlot->SetAutoSize(false);
				CanvasSlot->SetSize(MedicineIconSize);
			}
			MedicineIconImage->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			MedicineIconImage->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	if (MedicineCountText)
	{
		if (bHas)
		{
			MedicineCountText->SetText(FText::AsNumber(HC->HealthAmount[0]));
			MedicineCountText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			MedicineCountText->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UHUDWidget::OnGrenadeCookingStarted()
{
	// 开始拉引信 — 显示 cooking UI
	if (CookingTimeText)  CookingTimeText->SetVisibility(ESlateVisibility::Visible);
	if (CookingRingImage) CookingRingImage->SetVisibility(ESlateVisibility::Visible);
}

void UHUDWidget::RefreshGrenadeSlots()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return; 
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char)  return; 
	UGrenadeComponent* GC = Char->FindComponentByClass<UGrenadeComponent>();
	if (!GC) return; 

	if (!GrenadeSlotBox)  return; 
	if (!GrenadeSlotEntryClass) return; 

	// 动态按背包重建：只生成拥有的种类，空槽自然不出现（无空隙、自动重排）
	GrenadeSlotBox->ClearChildren();

	

	for (int32 i = 0; i < GC->GrenadeTypes.Num(); ++i)
	{
		UGrenadeSlotEntry* Entry = CreateWidget<UGrenadeSlotEntry>(this, GrenadeSlotEntryClass);
		if (!Entry)  continue; 

		const int32 Amount = GC->GrenadeAmounts.IsValidIndex(i) ? GC->GrenadeAmounts[i] : 0;
		Entry->SetupSlot(GC->GrenadeTypes[i], Amount, i == GC->CurrentGrenadeIndex);

		// WBP_GrenadeSlot 根是 CanvasPanel（不自报尺寸），用 SizeBox 强制每个槽位为 GrenadeSlotSize，
		// 再用 Automatic 让 HorizontalBox 按此尺寸排布（不拉伸、不塌缩）。
		USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		SizeBox->SetWidthOverride(GrenadeSlotSize.X);
		SizeBox->SetHeightOverride(GrenadeSlotSize.Y);
		SizeBox->AddChild(Entry);

		if (UHorizontalBoxSlot* BoxSlot = GrenadeSlotBox->AddChildToHorizontalBox(SizeBox))
		{
			BoxSlot->SetSize(ESlateSizeRule::Automatic);
			BoxSlot->SetHorizontalAlignment(HAlign_Center);
			BoxSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UHUDWidget::OnGrenadeCookingProgress(float Progress)
{
	// 烹饪被重置为 0（扔出/收回/死亡）时，隐藏烹饪环与倒计时
	if (Progress <= 0.0f)
	{
		if (CookingTimeText) CookingTimeText->SetVisibility(ESlateVisibility::Hidden);
		if (CookingRingImage)
		{
			CookingRingImage->SetVisibility(ESlateVisibility::Hidden);
			if (CookingRingMaterial)
				CookingRingMaterial->SetScalarParameterValue(FName("Progress"), 0.0f);
		}
		return;
	}

	if (CookingRingMaterial)
	{
		CookingRingMaterial->SetScalarParameterValue(FName("Progress"), Progress);
	}

	// 更新剩余秒数文字
	if (CookingTimeText)
	{
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
			if (Char)
			{
				UGrenadeComponent* GC = Char->FindComponentByClass<UGrenadeComponent>();
				if (GC && GC->bIsCooking)
				{
					UGrenadeData* Data = GC->GetCurrnetGrenade();
					if (Data)
					{
						float Remaining = Data->FusingTime * (1.0f - Progress);
						CookingTimeText->SetText(FText::Format(
							FText::FromString(TEXT("{0}")),
							FText::AsNumber(FMath::Max(0.0f, FMath::CeilToFloat(Remaining * 10.0f) / 10.0f))));
					}
				}
			}
		}
	}
}

void UHUDWidget::UpdateScoreDisplay(int32 Kills, int32 Deaths, int32 Scores)
{
	if (KillsText) KillsText->SetText(FText::AsNumber(Kills));
	if (DeathsText) DeathsText->SetText(FText::AsNumber(Deaths));
	if (ScoreText) ScoreText->SetText(FText::AsNumber(Scores));
}

void UHUDWidget::UpdateMatchTimeDisplay(float TimeRemaining)
{
	if (!MatchTimeText) return;
	int32 Minutes = FMath::FloorToInt32(TimeRemaining / 60.0f);
	int32 Seconds = FMath::FloorToInt32(FMath::Fmod(TimeRemaining, 60.0f));
	MatchTimeText->SetText(FText::Format(FText::FromString(TEXT("{0}:{1}")),
		FText::AsNumber(Minutes),
		(Seconds < 10 ? FText::Format(FText::FromString(TEXT("0{0}")), FText::AsNumber(Seconds)) : FText::AsNumber(Seconds))));
}

void UHUDWidget::OnKillPlayReceived(const FString& KillerName, const FString& VictimName, ETeam KillerTeam)
{
	if (!KillPlayBox || !KillPlayEntryClass) return;
	if (KillPlayBox->GetChildrenCount() >= MaxKillPlayEntries) {
		UUserWidget* Oldest = Cast<UUserWidget>(KillPlayBox->GetChildAt(0));
		if (Oldest) {
			RemoveEntryInterval(Oldest);
		}
	}
	UUserWidget* Entry = CreateWidget(GetWorld(), KillPlayEntryClass);
	if (!Entry)return;

	UFunction* SetFunction = Entry->FindFunction(FName("SetKillInfo"));
	if (SetFunction)
	{
		FKillPlayParams Params(KillerName, VictimName, KillerTeam);
		Entry->ProcessEvent(SetFunction, &Params);
	}
	else
	{

	}
	KillPlayBox->AddChildToVerticalBox(Entry);
	FTimerHandle FOnKillPlayHandle;
	GetWorld()->GetTimerManager().SetTimer(FOnKillPlayHandle, [this, Entry]() {RemoveEntryInterval(Entry);}, KillPlayLifetime, false);
	EntryTimerMap.Add(Entry, FOnKillPlayHandle);

}

void UHUDWidget::SetInteractionPrompt(bool bShow, const FString& Text)
{
	if (!DoorPromptText)return;
	if (bShow) {
		DoorPromptText->SetText(FText::FromString(Text));
		DoorPromptText->SetVisibility(ESlateVisibility::Visible);
	}
	else {
		DoorPromptText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FLinearColor UHUDWidget::ResolveKillFeedColor(ETeam KillerTeam) const
{
	if (KillerTeam == ETeam::None) return FLinearColor::White;

	EKillFeedColorMode Mode = EKillFeedColorMode::FixedByTeam;
	if (As1mpleFpsGameState* GS = GetWorld()->GetGameState<As1mpleFpsGameState>())
	{
		Mode = GS->KillFeedColorMode;
	}

	// 模式一：按击杀者实际队伍（蓝队→蓝、红队→红）
	if (Mode == EKillFeedColorMode::FixedByTeam)
	{
		return (KillerTeam == ETeam::Blue) ? KillFeedBlueColor : KillFeedRedColor;
	}

	// 模式二：相对观看者（自己队→蓝、敌方→红）
	ETeam MyTeam = ETeam::None;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (As1mpleFpsPlayerState* PS = PC->GetPlayerState<As1mpleFpsPlayerState>())
		{
			MyTeam = PS->Team;
		}
	}
	return (KillerTeam == MyTeam) ? KillFeedBlueColor : KillFeedRedColor;
}

void UHUDWidget::ToggleScoreboard()
{
	if (!ScoreboardPanel) return;
	bool bVisible = ScoreboardPanel->GetVisibility() == ESlateVisibility::Visible;
	if (bVisible)
	{
		ScoreboardPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	else
	{
		ScoreboardPanel->SetVisibility(ESlateVisibility::Visible);
		RefreshScoreboard();
	}
}

void UHUDWidget::RefreshScoreboard()
{
	if (!ScoreboardPlayerList) return;
	ScoreboardPlayerList->ClearChildren();

	UWorld* World = GetWorld();
	if (!World) return;

	AGameStateBase* GS = World->GetGameState<AGameStateBase>();
	if (!GS) return;

	// Collect players sorted by score
	TArray<APlayerState*> Players = GS->PlayerArray;
	Players.Sort([](const APlayerState& A, const APlayerState& B) {
		return A.GetScore() > B.GetScore();
		});

	for (APlayerState* Entry : Players)
	{
		As1mpleFpsPlayerState* PS = Cast<As1mpleFpsPlayerState>(Entry);
		if (!PS) continue;

		FString Row = FString::Printf(TEXT("%-16s  K:%d  D:%d  S:%d"),
			*PS->GetPlayerName(),
			PS->Kills,
			PS->Deaths,
			PS->Scores);

		UTextBlock* RowText = NewObject<UTextBlock>(this);
		RowText->SetText(FText::FromString(Row));
		RowText->SetJustification(ETextJustify::Left);
		RowText->SetColorAndOpacity(FSlateColor(FLinearColor(0.772f, 0.786f, 0.824f))); // #E4E6EB

		
		FSlateFontInfo Font = CustomFontInfo;
		Font.Size = 14;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		RowText->SetFont(Font);

		RowText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		RowText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		ScoreboardPlayerList->AddChildToVerticalBox(RowText);
	}
}