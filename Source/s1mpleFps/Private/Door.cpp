// Fill out your copyright notice in the Description page of Project Settings.


#include "Door.h"
#include "Net/UnrealNetwork.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "s1mpleFpsPlayerController.h"
#include "Engine/World.h"
// Sets default values
ADoor::ADoor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	RootComponent =Box;
	Box->SetBoxExtent(FVector(100.f, 100.f, 100.f));
	Box->SetGenerateOverlapEvents(true);
	Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Box->SetCollisionResponseToAllChannels(ECR_Overlap);

	DoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DoorMesh"));
	DoorMesh->SetupAttachment(RootComponent);

	TargetRotation = FRotator::ZeroRotator;
	TargetLocation = FVector::ZeroVector;
}

// Called when the game starts or when spawned
void ADoor::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddDynamic(this, &ADoor::OnTriggerBegin);
	Box->OnComponentEndOverlap.AddDynamic(this, &ADoor::OnTriggerEnd);
}

// Called every frame
void ADoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (!bMyLocalActive)return;
	bool bReach = false;
	if (Type == EOpenType::Rotate) {
		//actually 这里不需要防护,防护是bIsOpen决定的是否用TargetLocation
	
			FRotator Current = DoorMesh->GetRelativeRotation();
			FRotator NewRot = FMath::RInterpTo(Current, TargetRotation, DeltaTime,InterpSpeed);

			DoorMesh->SetRelativeRotation(NewRot);
			if (NewRot.Equals(TargetRotation, 0.1f)) {
				DoorMesh->SetRelativeRotation(TargetRotation);
				bReach = true;
			}
	}
	else if (Type == EOpenType::Translate) {
		FVector CurrentLocation = DoorMesh->GetRelativeLocation();
		FVector NewLoc = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, InterpSpeed);

		DoorMesh->SetRelativeLocation(NewLoc);
		if (NewLoc.Equals(TargetLocation, 0.1f)) {
			DoorMesh->SetRelativeLocation(TargetLocation);
			bReach = true;
		}
	}
	if (bReach) {
		bMyLocalActive = false;
		if (HasAuthority()) {
			bIsMoving = false;
		}
	}
}

void ADoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{	
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADoor, bIsOpen);
	DOREPLIFETIME(ADoor, bIsMoving);
	DOREPLIFETIME(ADoor, TargetLocation);
	DOREPLIFETIME(ADoor, TargetRotation);
}

void ADoor::Door_Interact(APlayerController* Player)
{
	As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Player);
	if (!PC)return;
	// 客户端不能在门(非拥有 actor)上直接发 Server RPC，会被 UNetDriver 丢弃（No owning connection）。
	// 改走 PlayerController 的 Server RPC：客户端拥有自己的 PC，能正常发送到服务器。
	PC->ServerInteractDoor(this);
}

void ADoor::OnRep_DoorState()
{

	if (!bMyLocalActive) {
		bMyLocalActive = true;
	}
}

void ADoor::OnTriggerBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor)) {
		if (As1mpleFpsPlayerController* PC = Cast<As1mpleFpsPlayerController>(Pawn->GetController()) ){
			InteractController = PC;
			if (PC->IsLocalController()) {
				PC->ClientUpdatePrompt(true, TEXT("E To Interact"));
			}
		}
	}

}

void ADoor::OnTriggerEnd(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!InteractController) return;

	// OtherActor 是离开触发区的 Pawn，用它的实际 Controller 判断是否该清掉提示。
	// 之前用 GetInstigatorController() 对普通走动的玩家永远返回 null（instigator 只在造成伤害时才设置），
	// 导致 InteractController 永远清不掉、提示「E To Interact」卡在屏幕上。
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (InteractController == Pawn->GetController())
		{
			if (InteractController->IsLocalController()) {
				InteractController->ClientUpdatePrompt(false, TEXT(" "));
			}
			InteractController = nullptr;
		}
	}
}

void ADoor::RequestOpen(APlayerController* Player)
{

	if (!Player) { return; }
	if (bIsMoving) { return; }

	APawn* CallingPawn = Player->GetPawn();
	if (!CallingPawn) { return; }

	// 直接检测玩家是否与Box重叠（基于盒体范围）
	if (!Box->IsOverlappingActor(CallingPawn))
	{
		return; // 不在盒体内，拒绝
	}

	bIsOpen = !bIsOpen;
	bIsMoving = true;
	if (Type == EOpenType::Rotate)
	{
		// ---------- 需求1：根据玩家位置决定往里/往外开 ----------
		// 注意：此计算仅在服务器进行，算出的 TargetRotation 复制给所有客户端
		FVector PlayerLoc = Player->GetPawn()->GetActorLocation();
		FVector DoorLoc = GetActorLocation();
		FVector DirToPlayer = (PlayerLoc - DoorLoc).GetSafeNormal2D(); // 水平方向

		// 获取门板的前向向量（假设门板正面朝前，可根据实际模型调整）
		FVector DoorForward = DoorMesh->GetForwardVector().GetSafeNormal2D();

		// 点积判断：正=玩家在正面，负=玩家在背面
		float Dot = FVector::DotProduct(DirToPlayer, DoorForward);
		float Sign = (Dot >= 0.0f) ? 1.0f : -1.0f;

		// 开门方向：正面朝里（正转），背面朝外（反转）
		TargetRotation = bIsOpen ? FRotator(0.0f, Sign * YawRotation, 0.0f) : FRotator::ZeroRotator;
		TargetLocation = FVector::ZeroVector; // 旋转模式无位移
	}
	else // Translate
	{
		// ---------- 需求2：平移滑动门 ----------
		// 关门回到零位，开门移动到 TranslateOffset（相对Box的局部坐标）
		TargetLocation = bIsOpen ? Location : FVector::ZeroVector;
		TargetRotation = FRotator::ZeroRotator; // 平移模式无旋转
	}

	OnRep_DoorState();
}

void ADoor::ServerOpenDoor_Implementation(As1mpleFpsPlayerController* PC)
{
	RequestOpen(PC);
}

