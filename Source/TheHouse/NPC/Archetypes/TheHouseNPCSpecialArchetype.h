// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCArchetype.h"
#include "TheHouseNPCSpecialArchetype.generated.h"

/**
 * PNJ « hors casino » ou scriptés : police, secours, médecin…
 * Pas de salaire RH ni prix d’entrée client par défaut — étendez cette classe ou ajoutez des champs
 * si un jour certains cas spéciaux ont besoin de données dédiées (permis, véhicule…).
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseNPCSpecialArchetype : public UTheHouseNPCArchetype
{
	GENERATED_BODY()

public:
	UTheHouseNPCSpecialArchetype();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Special|Role")
	ETheHouseNPCSpecialKind SpecialKind = ETheHouseNPCSpecialKind::None;

	/** Identifiant libre pour scripts / BT (ex. PATROL_ROUTE_A). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Special|Role")
	FName SpecialDutyId = NAME_None;
};
