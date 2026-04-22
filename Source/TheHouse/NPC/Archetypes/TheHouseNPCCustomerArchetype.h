// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCArchetype.h"
#include "TheHouseNPCCustomerArchetype.generated.h"

/**
 * Archetype visiteur / client : budget de jeu, et option d’admission (futur).
 * Staff et Special n’embarquent pas ces champs : un membre du personnel n’est pas un client
 * sur le plan de ce Data Asset.
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseNPCCustomerArchetype : public UTheHouseNPCArchetype
{
	GENERATED_BODY()

public:
	UTheHouseNPCCustomerArchetype();

	/** Portefeuille initial (copié vers Wallet sur ATheHouseNPCCharacter). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Customer|Economy", meta = (ClampMin = "0.0"))
	float StartingWallet = 100.f;

	/**
	 * Frais d’entrée éventuels au casino (placeholder pour gameplay futur).
	 * 0 = désactivé. Applicable aux clients uniquement au travers de votre logique de jeu.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Customer|Admission", meta = (ClampMin = "0.0"))
	float OptionalAdmissionFee = 0.f;
};
