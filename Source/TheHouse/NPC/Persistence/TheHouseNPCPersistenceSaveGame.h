#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TheHouseNPCPersistenceSubsystem.h"
#include "TheHouseNPCPersistenceSaveGame.generated.h"

/**
 * Sauvegarde des PNJ persistants (états simulés + identifiants).
 * Note: on sauvegarde des “tickets” (state) et pas les acteurs UE directement.
 */
UCLASS()
class THEHOUSE_API UTheHouseNPCPersistenceSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category="TheHouse|NPC|Persistence")
	TArray<FTheHouseNPCPersistentState> SavedStates;

	/** Seed pour rendre le “random spawn” reproductible après chargement (si tu l’utilises). */
	UPROPERTY(VisibleAnywhere, Category="TheHouse|NPC|Persistence")
	int32 RandomSeed = 0;
};

