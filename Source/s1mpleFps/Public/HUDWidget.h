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

class AControlArea;
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

	// --- 记分板：两队分两侧（左蓝右红）。团队头 + 各队玩家列表，具体摆位在 WBP_HUD 里做 ---
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* BlueTeamHeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* RedTeamHeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* BlueTeamPlayerList;

	UPROPERTY(meta = (BindWidgetOptional))
	UVerticalBox* RedTeamPlayerList;

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

	// --- 占点进度 UI（容器 + 进度条；具体外观在 WBP_HUD 里自己设计） ---
	// 有点位被激活时才显示，否则折叠隐藏（NativeTick 里自动控制可见性）
	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel* ControlPointPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UProgressBar* ControlPointProgressBar;
	// -------------------

	// --- 占点广播文本：谁占领了据点（短暂显示后自动隐藏，颜色按队伍） ---
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ControlPointBroadcastText;

	// 广播显示时长（秒）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ControlPoint")
	float ControlPointBroadcastLifetime = 3.0f;

	// 队伍颜色：团队分 / 占点广播等按队伍着色用（蓝图可调，改这里即可改样式）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FLinearColor TeamBlueColor = FLinearColor(0.25f, 0.6f, 1.0f, 1.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FLinearColor TeamRedColor = FLinearColor(1.0f, 0.25f, 0.25f, 1.0f);

	// 记分板队名（显示在团队头第一行）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FString BlueTeamName = TEXT("VALOIS");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	FString RedTeamName = TEXT("PLANTAGENET");

	// 记分板玩家行样式（行文本在 C++ 里动态生成，所以暴露这些给蓝图改字体/字号/颜色）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	UFont* ScoreboardFont = nullptr;   // 为空则用 NativeConstruct 里加载的 CustomFont

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	int32 ScoreboardFontSize = 14;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scoreboard")
	FLinearColor ScoreboardRowColor = FLinearColor(0.772f, 0.786f, 0.824f);

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

	// 返回当前激活的占点点位（没有激活的点返回 nullptr）；供蓝图读队伍/进度/自己做外观
	UFUNCTION(BlueprintCallable)
	AControlArea* GetActiveControlArea() const;

	// 占点得分广播回调（谁占领了据点 + 本次得分）；短暂显示广播文本并按队伍着色
	UFUNCTION()
	void OnControlPointScoredReceived(ETeam Team, int32 Score);
protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
private:
	void RefreshScoreboard();
	void AddScoreboardRow(UVerticalBox* List, const FString& Text);
	void TryBindPlayerState();
	void UpdateControlPointDisplay();

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
	bool bControlPointScoredBound = false;

	bool bIsHealingActive = false;
	float HealingEndTime = 0.0f;
	float HealingTotalDuration = 0.0f;

	FLinearColor HealthGreen;
	FLinearColor HealthYellow;
	FLinearColor HealthRed;

	TMap<UUserWidget*, FTimerHandle> EntryTimerMap;
	TMap<UTextBlock*, FTimerHandle> ChatEntryTimerMap;
	FTimerHandle WarmUpHideHandle;
	FTimerHandle ControlPointBroadcastHandle;
	void RemoveEntryInterval(UUserWidget* Entry);
	void OnEntryTimerElapsed(UUserWidget* Entry);
	FLinearColor ResolveKillFeedColor(ETeam KillerTeam) const;
	FLinearColor ResolveTeamColor(ETeam Team) const;
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
