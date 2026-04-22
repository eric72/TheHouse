// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TheHouse/NPC/TheHouseNPCTypes.h"
#include "TheHouseNPCSubsystem.generated.h"

class ATheHouseNPCCharacter;

/**
 * Registre monde des PNJ pour requêtes par catégorie (personnel, clients, spéciaux) sans scanner
 * toute la scène. Les ATheHouseNPCCharacter s’inscrivent au BeginPlay et se désinscrivent au
 * EndPlay — ne pas appeler Register à la main sauf cas exceptionnel (tests unitaires, outils).
 */
UCLASS()
class THEHOUSE_API UTheHouseNPCSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void RegisterNPC(ATheHouseNPCCharacter* NPC);
	void UnregisterNPC(ATheHouseNPCCharacter* NPC);

	/** Liste copiée et compactée (sans entrées invalides). */
	UFUNCTION(BlueprintCallable, Category = "NPC|Registry")
	TArray<ATheHouseNPCCharacter*> GetNPCsByCategory(ETheHouseNPCCategory Category) const;

	UFUNCTION(BlueprintCallable, Category = "NPC|Registry")
	int32 GetNPCCountByCategory(ETheHouseNPCCategory Category) const;

private:
	using FNPCWeakList = TArray<TWeakObjectPtr<ATheHouseNPCCharacter>>;

	/** Non sérialisé : reconstruction au chargement via Register sur BeginPlay des PNJ. */
	TMap<ETheHouseNPCCategory, FNPCWeakList> NPCsByCategory;

	void CompactBucket(ETheHouseNPCCategory Category);
};
