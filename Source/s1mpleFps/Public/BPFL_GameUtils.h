#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "InputCoreTypes.h"
#include "BPFL_GameUtils.generated.h"

UCLASS()
class S1MPLEFPS_API UBPFL_GameUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Game")
	static void RestartGame(UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category = "Game")
	static void QuitToDesktop(UObject* WorldContext);

	UFUNCTION(BlueprintCallable)
	static void HostGame(UObject* WorldContext, const FString& MapName);
	UFUNCTION(BlueprintCallable)
	static void JoinGame(UObject* WorldContext, const FString& IPAddress);

	// Player Mappable Input 便捷改键：MappingName = InputAction 的 Player Mappable Display Name
	UFUNCTION(BlueprintCallable, Category = "Input|Player Mappable", meta = (WorldContext = "WorldContext"))
	static bool RemapKey(UObject* WorldContext, FName MappingName, FKey NewKey, FString& OutError);
};
