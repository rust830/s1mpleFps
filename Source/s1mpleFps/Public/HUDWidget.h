#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "HUDWidget.generated.h"

class UTP_WeaponComponent;
class UGrenadeComponent;
class UGrenadeData;
class UMaterialInstanceDynamic;
class UMaterialInterface;

UCLASS()
class S1MPLEFPS_API UHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// --- BindWidget elements (names must match Blueprint) ---
	UPROPERTY(meta = (BindWidget))
	UImage* CrosshairImage;

	UPROPERTY(meta = (BindWidget))
	UImage* HitMarkerImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AmmoText;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealthPercentText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* KillsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* DeathsText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ScoreText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchTimeText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WarmUpCountdownText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* WeaponNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* ArmorNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* HealingText;

	// --- Grenade UI ---
	UPROPERTY(meta = (BindWidget))
	UImage* GrenadeIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* GrenadeCountText;

	UPROPERTY(meta = (BindWidget))
	UImage* CookingRingImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CookingTimeText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	UMaterialInterface* CookingRingBaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* CookingRingMaterial;

	// --- 打药进度环 UI（复用 M_CookingRing 材质） ---
	UPROPERTY(meta = (BindWidget))
	UImage* HealingRingImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
	UMaterialInterface* HealingRingBaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* HealingRingMaterial;

	// 打药环颜色（白色环 × 此 tint = 目标颜色，材质复用 M_CookingRing 不用复制）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
	FLinearColor HealingRingColor = FLinearColor(0.0f, 1.0f, 0.5f, 1.0f);

	UFUNCTION(BlueprintCallable)
	void UpdateGrenadeDisplay(UGrenadeData* Data, int32 Count);

	UFUNCTION()
	void OnGrenadeCookingStarted();

	UFUNCTION()
	void OnGrenadeCookingProgress(float Progress);

	UFUNCTION()
	void OnGrenadeEquippedChanged();

	UFUNCTION()
	void OnGrenadeInventoryChanged();

	void BindToGrenadeComponent();
	bool bGrenadeBound = false;

	// 热身倒计时样式 — 蓝图可调，C++ 兜底字号
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WarmUp")
	int32 WarmUpFontSize = 96;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WarmUp")
	float WarmUpDisplayTime = 0.8f;

	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* ScoreboardPanel;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* ScoreboardPlayerList;

	UPROPERTY(meta = (BindWidget))
	UVerticalBox* KillPlayBox;

	UPROPERTY(BlueprintReadWrite,meta = (BindWidget))
	UVerticalBox* ChatMessageBox;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
	int32 MaxChatMessages = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chat")
	float ChatMessageLifetime = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
	int32 MaxKillPlayEntries = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
	float KillPlayLifetime = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
	TSubclassOf<UUserWidget> KillPlayEntryClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	USoundBase* OvertimeTickSound;


	// --- Match End UI ---
	UPROPERTY(meta = (BindWidget))
	UCanvasPanel* MatchEndPanel;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchEndResultText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MatchEndInfoText;

	UPROPERTY(meta = (BindWidget))
	UButton* RestartButton;

	UPROPERTY(meta = (BindWidget))
	UButton* LeaveButton;
	// -------------------

	// --- Public functions ---
	UFUNCTION(BlueprintCallable)
	void PlayHitMarker(bool bIsEnemy);

	UFUNCTION(BlueprintCallable)
	void BindToWeapon(UTP_WeaponComponent* Weapon);

	UFUNCTION(BlueprintCallable)
	void ToggleScoreboard();

	UFUNCTION(BlueprintCallable)
	void UpdateHealthDisplay(float CurrentHealth, float MaxHealth);

	UFUNCTION(BlueprintCallable)
	void UpdateAmmoDisplay(int32 CurrentAmmo, int32 SpareAmmo);

	UFUNCTION(BlueprintCallable)
	void UpdateHealingDisplay(bool bIsHealing, float Duration);

	UFUNCTION(BlueprintCallable)
	void UpdateEquipmentDisplay();

	UFUNCTION(BlueprintCallable)
	void UpdateScoreDisplay(int32 Kills, int32 Deaths, int32 Scores);

	UFUNCTION(BlueprintCallable)
	void UpdateMatchTimeDisplay(float TimeRemaining);

	UFUNCTION(BlueprintCallable)
	void OnKillPlayReceived(const FString& KillerName, const FString& VictimName);

	UFUNCTION()
	void OnChatMessageReceived(const FString& Sender, const FString& Message, bool bIsTeam);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
private:
	void RefreshScoreboard();
	void TryBindPlayerState();

	float HitMarkerAlpha;
	float HitMarkerDuration;
	bool bHitMarkerIsEnemy;

	bool bPlayerStateBound = false;
	bool bGameStateBound = false;
	bool bKillPlayBound = false;
	bool bMatchEndBound = false;
	bool bSuddenDeathBound = false;
	bool bOvertimeBound = false;
	bool bWarmUpBound = false;
	bool bChatMessageBound = false;

	bool bIsHealingActive = false;
	float HealingEndTime = 0.0f;
	float HealingTotalDuration = 0.0f;

	FLinearColor HealthGreen;
	FLinearColor HealthYellow;
	FLinearColor HealthRed;

	TMap<UUserWidget*, FTimerHandle> EntryTimerMap;
	TMap<UTextBlock*, FTimerHandle> ChatEntryTimerMap;
	FTimerHandle WarmUpHideHandle;
	void RemoveEntryInterval(UUserWidget* Entry);
	void OnEntryTimerElapsed(UUserWidget* Entry);
	UFUNCTION()
	void OnMatchEndedReceived(const FString& WinnerName, bool bWinByKill);
	UFUNCTION()
	void OnSuddenDeathReceived();
	UFUNCTION()
	void OnOvertimeReceived(float OvertimeRemaining);
	UFUNCTION()
	void OnWarmUpReceived(float WarmUpTimeRemaining);

	UFUNCTION()
	void RestartMatch();

	UFUNCTION()
	void LeaveMatch();
};
