// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TheHouseNPCTypes.generated.h"

/**
 * Grande famille de PNJ utilisée pour le routage système (économie, registre, règles de jeu).
 * Ne remplace pas un identifiant de métier fin (voir FName StaffRoleId sur l’archetype) : ce
 * regroupement sert à savoir quel “paquet” de règles et d’UI s’applique au personnage.
 */
UENUM(BlueprintType)
enum class ETheHouseNPCCategory : uint8
{
	/** Instance pas encore configurée ou archetype non assigné (état invalide à corriger). */
	Unassigned UMETA(Hidden),

	/** Personnel interne : coûts de salaire, fonctions dédiées, pas de comportement “client”. */
	Staff UMETA(DisplayName = "Personnel"),

	/** Visiteur : budget, interaction avec les jeux/objets réservés aux clients. */
	Customer UMETA(DisplayName = "Client"),

	/** Acteurs “spéciaux” : services publics, urgences, scénarios scriptés… */
	Special UMETA(DisplayName = "Special")
};

/**
 * Sous-type pour la catégorie Special uniquement (police, secours…).
 * Étendez la liste au fil des besoins narrative / gameplay.
 */
UENUM(BlueprintType)
enum class ETheHouseNPCSpecialKind : uint8
{
	None UMETA(DisplayName = "Aucun"),
	Police UMETA(DisplayName = "Police"),
	Firefighter UMETA(DisplayName = "Pompier"),
	Doctor UMETA(DisplayName = "Médecin"),
	Other UMETA(DisplayName = "Autre")
};
