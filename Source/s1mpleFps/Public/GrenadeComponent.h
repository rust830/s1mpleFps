// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GrenadeData.h"
#include "GrenadeComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrenadeEquipped);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnGrenadeChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCookingStart);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCookingProgressChanged, float, Progress);

class As1mpleFpsCharacter;
class UInputMappingContext;
class UInputAction;
class UCameraComponent;
class UStaticMeshComponent;
class UInstancedStaticMeshComponent;

UCLASS(ClassGroup=(Custom), Blueprintable, meta=(BlueprintSpawnableComponent))
class S1MPLEFPS_API UGrenadeComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UGrenadeComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
	void ToggleGrenadeMode();
	void OnStartHighThrowMode();
	void OnStartLowThrowMode();
	void OnStopThrow();
	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Input")
	UInputMappingContext* IMC_Grenade;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* HighThrowAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* LowThrowAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* NextGrenadeAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	UInputAction* PrevGrenadeAction;



	UPROPERTY(ReplicatedUsing=OnRep_GrenadeInventory, EditDefaultsOnly, BlueprintReadOnly)
	TArray<UGrenadeData*> GrenadeTypes;
	UPROPERTY(ReplicatedUsing = OnRep_GrenadeInventory, EditDefaultsOnly, BlueprintReadOnly)
	TArray<int32> GrenadeAmounts;
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentGrenadeIndex = 0;
	
	UFUNCTION(BlueprintCallable)
	bool HasGrenade()const;
	UFUNCTION(BlueprintCallable)
	UGrenadeData* GetCurrnetGrenade();
	UFUNCTION(Server,Reliable,BlueprintCallable)
	void ServerAddGrenade(UGrenadeData* Grenade, int32 Amount);
	
	void AddGrenade(UGrenadeData* Grenade, int32 Amount);
	UPROPERTY(BlueprintReadOnly)
	bool bIsEquipped = false;
	UPROPERTY(BlueprintReadOnly)
	bool bIsLowThrow = false;
	UPROPERTY(BlueprintReadOnly)
	bool bIsCooking = false;
	UPROPERTY(BlueprintReadOnly)
	float CookingElapsed = 0.0f;
	UPROPERTY(EditAnywhere,BlueprintReadOnly)
	FName ThrowSocketName = TEXT("hand_r");

	// 捏雷视觉的本地相对变换（在右手 socket 上的微调）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	FVector HeldGrenadeRelativeLocation = FVector(0.f, 0.f, 0.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	FRotator HeldGrenadeRelativeRotation = FRotator::ZeroRotator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grenade")
	FVector HeldGrenadeRelativeScale = FVector(1.f, 1.f, 1.f);

	// 落点预测线（本地可见，点状虚线）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	UStaticMesh* TrajectoryDotMesh = nullptr;    // 沿途的小圆点
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	UStaticMesh* TrajectoryLandMesh = nullptr;   // 落点标记（环/叉）
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	float DotSpacing = 25.f;                     // 点间距
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trajectory")
	float DotScale = 0.08f;                      // 点的缩放（取决于网格尺寸）


	UPROPERTY(BlueprintAssignable)
	FOnGrenadeEquipped OnGrenadeEquipped;
	UPROPERTY(BlueprintAssignable)
	FOnGrenadeChanged OnGrenadeInventoryChanged;
	UPROPERTY(BlueprintAssignable)
	FOnCookingStart OnCookingStart;
	UPROPERTY(BlueprintAssignable)
	FOnCookingProgressChanged OnCookingProgressChanged;

	void LeaveGrenadeMode();
	void ForceUnequip();  // 死亡时强制收回（无视 bIsCooking）
	void RemoveGrenadeMappingContext();

	// 计算当前投掷初速度（高抛/低抛共用），供 PerformThrowGrenade 与落点预测复用
	FVector ComputeThrowVelocity();

	UFUNCTION()
	void NextGrenade();
	UFUNCTION()
	void PrevGrenade();
protected:
	void EquippedGrenade();
	void UnequippedGrenade();
	void PerformThrowGrenade();
	void OnStartCooking();
	void AddGrenadeMappingContext();
	void ShowHeldGrenade();
	void HideHeldGrenade();
	void UpdateTrajectoryPreview();

	UFUNCTION()
	void OnRep_GrenadeInventory();
	UFUNCTION(Server,Reliable)
	void ServerThrowGrenade(int32 GrenadeIndex, FVector Velocity, float RemainingTime);
	UFUNCTION(NetMulticast,Reliable)
	void NetMulticastThrowSound(UGrenadeData* Data,FVector Velocity,float RemainingTime);

	void OnCookingExpired();

	UPROPERTY()
	As1mpleFpsCharacter* OwnerCharacter;
	UPROPERTY()
	UCameraComponent* CachedCamera;
	// 右手捏着的本地雷（静态网格组件，无碰撞、无初速、仅本地可见）
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> HeldGrenadeMesh;
	// 落点预测线渲染组件（纯本地，运行时创建、不复制）
	UPROPERTY(Transient)
	TObjectPtr<UInstancedStaticMeshComponent> TrajectoryDots;
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> LandingMarker;
};
