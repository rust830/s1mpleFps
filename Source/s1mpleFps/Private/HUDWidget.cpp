#include "HUDWidget.h"
#include "TP_WeaponComponent.h"
#include "s1mpleFpsPlayerState.h"
#include "s1mpleFpsGameState.h"
#include "s1mpleFpsPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "s1mpleFpsCharacter.h"
#include "DamageComponent.h"
#include "ArmorData.h"
#include "WeaponData.h"
#include "GrenadeComponent.h"
#include "GrenadeData.h"
#include "GameFramework/PlayerState.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Materials/MaterialInstanceDynamic.h"


struct FKillPlayParams {
	 FString KillerName;
	 FString VictimName;
	 FKillPlayParams(const FString& InKillerName, const FString& InVictimName):KillerName(InKillerName),VictimName(InVictimName){}
};
void UHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	HitMarkerAlpha = 0.0f;
	HitMarkerDuration = 0.0f;
	bHitMarkerIsEnemy = false;

	HealthGreen  = FLinearColor(0.061f, 0.729f, 0.218f); // #4ADE80
	HealthYellow = FLinearColor(0.963f, 0.516f, 0.019f); // #FBBF24
	HealthRed    = FLinearColor(0.786f, 0.042f, 0.056f); // #E63946

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

	// 热身倒计时：蓝图中字号/颜色/位置均可直接配置，C++ 只兜底字号
	if (WarmUpCountdownText)
	{
		FSlateFontInfo WarmUpFont = WarmUpCountdownText->GetFont();
		if (WarmUpFont.Size <= 0) WarmUpFont.Size = WarmUpFontSize;
		WarmUpCountdownText->SetFont(WarmUpFont);
		WarmUpCountdownText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Hide hit marker, scoreboard, and ammo initially
	if (HitMarkerImage) HitMarkerImage->SetOpacity(0.0f);
	if (ScoreboardPanel) ScoreboardPanel->SetVisibility(ESlateVisibility::Collapsed);
	if (AmmoText) AmmoText->SetVisibility(ESlateVisibility::Hidden);
	if (HealingText) HealingText->SetVisibility(ESlateVisibility::Hidden);
	if (HealingRingImage)
	{
		HealingRingImage->SetVisibility(ESlateVisibility::Hidden);
		// 临时调试：去掉 tint，排除颜色叠加导致不可见的可能（与 CookingRing 保持一致）
		// HealingRingImage->SetColorAndOpacity(HealingRingColor);
		if (HealingRingBaseMaterial)
		{
			HealingRingMaterial = UMaterialInstanceDynamic::Create(HealingRingBaseMaterial, this);
			HealingRingImage->SetBrushFromMaterial(HealingRingMaterial);
			HealingRingMaterial->SetScalarParameterValue(FName("Progress"), 0.0f);
		}
	}

	// Grenade UI — hidden initially, shown when player has grenades; cooking only shown during cooking
	if (GrenadeIconImage) GrenadeIconImage->SetVisibility(ESlateVisibility::Hidden);
	if (GrenadeCountText) GrenadeCountText->SetVisibility(ESlateVisibility::Hidden);
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
		Character->HideDeathWidget();
	}
	bool bIsWinner = PC && PC->PlayerState && PC->PlayerState->GetPlayerName() == WinnerName;

	if (MatchEndPanel)
	{
		MatchEndPanel->SetVisibility(ESlateVisibility::Visible);
	}

	if (MatchEndResultText)
	{
		MatchEndResultText->SetText(FText::FromString(bIsWinner ? TEXT("胜利") : TEXT("失败")));
	}

	if (MatchEndInfoText)
	{
		FString Reason = bWinByKill ? TEXT("到达击杀上限") : TEXT("时间结束");
		FString Info = FString::Printf(TEXT("胜者：%s（%s）"), *WinnerName, *Reason);
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
		? FText::Format(FText::FromString(TEXT("加时赛-争者留其名!\n {0}:0{1}")), FText::AsNumber(Minutes), FText::AsNumber(Seconds))
		: FText::Format(FText::FromString(TEXT("加时赛-争者留其名!\n {0}:{1}")), FText::AsNumber(Minutes), FText::AsNumber(Seconds));

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
	UGameplayStatics::OpenLevel(this, TEXT("StartMap"));
}

void UHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Retry binding if PlayerState/GameState wasn't ready during NativeConstruct
	if (!bPlayerStateBound || !bGameStateBound||!bKillPlayBound||!bMatchEndBound||!bSuddenDeathBound||!bOvertimeBound||!bWarmUpBound||!bChatMessageBound||!bGrenadeBound)
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

	const FString Prefix = bIsTeam ? TEXT("[队聊] ") : TEXT("");
	const FString FullText = Prefix + Sender + TEXT(": ") + Message;
	const FLinearColor TeamChatBlue = FLinearColor(0.133f, 0.452f, 1.0f); // #66B2FF
	const FLinearColor Color = bIsTeam ? TeamChatBlue : FLinearColor::White;

	UTextBlock* TextEntry = NewObject<UTextBlock>(this);
	TextEntry->SetText(FText::FromString(FullText));
	TextEntry->SetColorAndOpacity(FSlateColor(Color));

	FSlateFontInfo Font = TextEntry->GetFont();
	Font.Size = 14;
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
		if (Char->CurrentWeapon && Char->CurrentWeapon->WeaponData)
		{
			const FString& Name = Char->CurrentWeapon->WeaponData->WeaponName;
			// Fall back to asset name if WeaponName field is empty (unfilled data asset).
			WeaponNameText->SetText(FText::FromString(Name.IsEmpty() ? Char->CurrentWeapon->WeaponData->GetName() : Name));
		}
		else if (Char->CurrentWeapon && !Char->CurrentWeapon->WeaponData)
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
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;
	As1mpleFpsCharacter* Char = Cast<As1mpleFpsCharacter>(PC->GetPawn());
	if (!Char) return;
	UGrenadeComponent* GC = Char->FindComponentByClass<UGrenadeComponent>();
	if (!GC) return;

	UGrenadeData* Data = GC->GetCurrnetGrenade();
	UpdateGrenadeDisplay(Data, GC->HasGrenade() ? GC->GrenadeAmounts[GC->CurrentGrenadeIndex] : 0);
}

void UHUDWidget::OnGrenadeCookingStarted()
{
	// 开始拉引信 — 显示 cooking UI
	if (CookingTimeText)  CookingTimeText->SetVisibility(ESlateVisibility::Visible);
	if (CookingRingImage) CookingRingImage->SetVisibility(ESlateVisibility::Visible);
}

void UHUDWidget::UpdateGrenadeDisplay(UGrenadeData* Data, int32 Count)
{
	const bool bHasGrenade = Count > 0;
	const ESlateVisibility Vis = bHasGrenade ? ESlateVisibility::Visible : ESlateVisibility::Hidden;

	if (GrenadeIconImage)
	{
		if (bHasGrenade && Data && Data->GrenadeIcon)
			GrenadeIconImage->SetBrushFromTexture(Data->GrenadeIcon);
		GrenadeIconImage->SetVisibility(Vis);
	}
	if (GrenadeCountText)
	{
		GrenadeCountText->SetText(FText::AsNumber(Count));
		GrenadeCountText->SetVisibility(Vis);
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

void UHUDWidget::OnKillPlayReceived(const FString& KillerName, const FString& VictimName)
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
		FKillPlayParams Params(KillerName, VictimName);
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

		FSlateFontInfo Font = RowText->GetFont();
		Font.Size = 14;
		Font.OutlineSettings.OutlineSize = 1;
		Font.OutlineSettings.OutlineColor = FLinearColor::Black;
		RowText->SetFont(Font);

		RowText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		RowText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
		ScoreboardPlayerList->AddChildToVerticalBox(RowText);
	}
}
