// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TheHouse/NPC/TheHouseNPCTypes.h"
#include "TheHouseNPCArchetype.generated.h"

/**
 * Racine des archetypes PNJ : données partagées + catégorie (remplie par les sous-classes Staff / Customer / Special).
 * En pratique : créez toujours des Data Assets **Staff**, **Customer** ou **Special** — pas d’asset « racine » seul.
 *
 * Note moteur : la classe n’est pas `Abstract` afin que le sélecteur d’assets du détail Unreal liste bien les
 * sous-types assignables sur `TObjectPtr<UTheHouseNPCArchetype>` (sinon le picker peut rester vide).
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseNPCArchetype : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Libellé affiché (UI, debug). Préférer FText + table de localisation pour le produit. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Identity")
	FText DisplayName;

	/**
	 * Renseigné uniquement par le constructeur des sous-classes ; permet au registre et à l’UI
	 * de connaître la catégorie sans Cast systématique.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Identity")
	ETheHouseNPCCategory Category = ETheHouseNPCCategory::Unassigned;
};
