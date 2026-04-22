// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TheHouse/NPC/TheHouseNPCTypes.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCArchetype.h"
#include "TheHouseNPCIdentity.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UTheHouseNPCIdentity : public UInterface
{
	GENERATED_BODY()
};

/**
 * Contrat commun pour tout acteur considéré comme PNJ du casino (interrogeable par l’UI,
 * l’économie, les sous-systèmes sans connaître la sous-classe concrète).
 */
class THEHOUSE_API ITheHouseNPCIdentity
{
	GENERATED_BODY()

public:
	/** Catégorie runtime (issue de l’archetype si celui-ci est défini). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NPC|Identity")
	ETheHouseNPCCategory GetNPCCategory() const;

	/** Archetype de données (nullptr possible si pas encore configuré). */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "NPC|Identity")
	UTheHouseNPCArchetype* GetNPCArchetype() const;
};
