#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Components/CanvasPanel.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "s1mpleFpsPlayerState.h"
#include "HUDWidget.generated.h"

class UTP_WeaponComponent;
class UGrenadeComponent;
class UGrenadeData;
class UGrenadeSlotEntry;
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

	// --- 药品图标 + 数量（常驻显示，类似手雷槽） ---
	UPROPERTY(meta = (BindWidget))
	UImage* MedicineIconImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* MedicineCountText;

	// 药品图标渲染尺寸（像素），配合 SetDesiredSizeOverride 防止被贴图分辨率撑大
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
	FVector2D MedicineIconSize = FVector2D(40.f, 40.f);

	// --- Grenade UI ---
	// 手雷槽容器（横向排布，按背包拥有的种类动态生成条目）
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* GrenadeSlotBox;

	// 单个手雷槽的 Widget 类（图标 + 数量 + 选中高亮）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	TSubclassOf<UGrenadeSlotEntry> GrenadeSlotEntryClass;

	// 单个手雷槽的固定尺寸（像素）。WBP_GrenadeSlot 根若是 CanvasPanel 无法自定尺寸，
	// 由这里用 SizeBox 在代码里强制槽位大小。改这里即可调整图标大小。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	FVector2D GrenadeSlotSize = FVector2D(40.f, 40.f);

	UPROPERTY(meta = (BindWidget))
	UImage* CookingRingImage;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* CookingTimeText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	UMaterialInterface* CookingRingBaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* CookingRingMaterial;

	UPROPERTY(meta = (BindWidget))
	UImage* HealingRingImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Healing")
	UMaterialInterface* HealingRingBaseMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* HealingRingMaterial;
	UPROPERTY(EditAnywhere,BlueprintReadWrite)
	UFont* CustomFont;

	FSlateFontInfo CustomFontInfo;
	//
	// 重建手雷槽列表（按 GrenadeTypes/GrenadeAmounts 生成条目，并高亮 CurrentGrenadeIndex）
	void RefreshGrenadeSlots();

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

	// 药品图标 + 数量刷新（绑定 HealthComponent 的 OnHealthItemsChanged）
	void RefreshMedicineDisplay();
	void BindToHealthComponent();
	bool bHealthBound = false;
	UFUNCTION()
	void OnHealthItemsChanged();

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

	// 击杀条颜色（两种模式下共用：蓝=友方/蓝队，红=敌方/红队）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
	FLinearColor KillFeedBlueColor = FLinearColor(0.25f, 0.6f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "KillFeed")
	FLinearColor KillFeedRedColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget))
	UTextBlock* DoorPromptText;
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
	void OnKillPlayReceived(const FString& KillerName, const FString& VictimName, ETeam KillerTeam);

	UFUNCTION(BlueprintCallable)
	void SetInteractionPrompt(bool bShow, const FString& Text);

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
	FLinearColor ResolveKillFeedColor(ETeam KillerTeam) const;
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
