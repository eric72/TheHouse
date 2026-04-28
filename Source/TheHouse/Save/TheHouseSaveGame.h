#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TheHouse/UI/TheHouseRTSUITypes.h"
#include "TheHouse/NPC/Persistence/TheHouseNPCPersistenceSubsystem.h"
#include "TheHouseSaveGame.generated.h"

class ATheHouseObject;

USTRUCT(BlueprintType)
struct FTheHouseSavedPlacedObject
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Save")
	TSoftClassPtr<ATheHouseObject> ObjectClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Save")
	FTransform WorldTransform;

	// Settings principaux que tu exposes déjà dans ATheHouseObject (si tu les changes runtime via UI).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Save")
	bool bNpcMoneyFlowEnabled = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Save")
	int32 NpcSpendToPlay = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Save")
	int32 NpcMaxReturnToHouse = 0;
};

/**
 * Sauvegarde globale (argent, objets placés, stock, PNJ persistants).
 * Objectif : “tout ce qui compte”, sans sérialiser directement les Actors.
 */
UCLASS()
class THEHOUSE_API UTheHouseSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	// --- Joueur / économie ---
	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|Player")
	int32 Money = 0;

	// --- Temps / cycle jour-nuit ---
	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|Time")
	float InGameTimeSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|Time")
	int32 InGameDayIndex = 0;

	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|Player")
	TArray<FTheHouseStoredStack> StoredPlaceableStacks;

	// --- Objets placés (casino + map) ---
	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|World")
	TArray<FTheHouseSavedPlacedObject> PlacedObjects;

	// --- PNJ (état persistant inter-niveaux) ---
	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|NPC")
	TArray<FTheHouseNPCPersistentState> NPCStates;

	UPROPERTY(VisibleAnywhere, Category="TheHouse|Save|NPC")
	int32 NPCRandomSeed = 0;
};

