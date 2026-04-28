#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TheHouseSaveSubsystem.generated.h"

class UTheHouseSaveGame;
class ATheHousePlayerController;

/**
 * Point central Save/Load “tout ce qui compte”.
 * Appelable en Blueprint (menu pause, debug, etc.).
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TheHouse|Save")
	bool SaveAll(const FString& SlotName = TEXT("TheHouseMain"), int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="TheHouse|Save")
	bool LoadAll(const FString& SlotName = TEXT("TheHouseMain"), int32 UserIndex = 0);

private:
	ATheHousePlayerController* GetHousePC(UWorld* World) const;
};

