// Fill out your copyright notice in the Description page of Project Settings.


#include "BTTask_Reload.h"
#include "EnemyAIController.h"
#include "EnemyCharacter.h"
#include "WeaponData.h"

UBTTask_Reload::UBTTask_Reload()
{
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_Reload::ExecuteTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory)
{   
	AAIController* AI = Cast<AAIController>(OwnerComponent.GetOwner());
	if (!AI) return EBTNodeResult::Failed;
	AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AI->GetPawn());
	
	if (!Enemy)return EBTNodeResult::Failed;
	FBTReloadMemoryNode* Memory = reinterpret_cast<FBTReloadMemoryNode*>(NodeMemory);
	Memory->TimeRemaining = (Enemy->EnemyWeapon)?Enemy->EnemyWeapon->ReloadTime:0.5f;
	Memory->bHasReloaded = false;

	return EBTNodeResult::InProgress;
}

void UBTTask_Reload::TickTask(UBehaviorTreeComponent& OwnerComponent, uint8* NodeMemory, float DeltaTime)
{
    FBTReloadMemoryNode* Memory = reinterpret_cast<FBTReloadMemoryNode*>(NodeMemory);
    Memory->TimeRemaining -= DeltaTime;

    // 真正装弹逻辑只执行一次
    if (!Memory->bHasReloaded)
    {
        AAIController* AI = OwnerComponent.GetAIOwner();
        if (AI)
        {
            AEnemyCharacter* Enemy = Cast<AEnemyCharacter>(AI->GetPawn());
            if (Enemy && Enemy->EnemyWeapon)
            {
                int32 Needed = Enemy->MaxAmmo - Enemy->CurrentAmmo;
                int32 ToReload = FMath::Min(Needed, Enemy->TotalAmmo);
                Enemy->CurrentAmmo += ToReload;
                Enemy->TotalAmmo -= ToReload;
            }
            else if (Enemy && Enemy->TotalAmmo <= 0)
            {
                Memory->TimeRemaining = 0.0f;
            }
        }
        Memory->bHasReloaded = true;
    }

    // 等待装弹时间结束
    if (Memory->TimeRemaining <= 0.f)
    {
        FinishLatentTask(OwnerComponent, EBTNodeResult::Succeeded);
    }
}
