// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Actor.h"
#include "TheHouseNPCEjectRegionVolume.generated.h"

/**
 * Volume de niveau (boîte orientée) définissant une « zone » d’où l’on peut ordonner l’éjection d’un client.
 * Placez l’acteur dans la scène, ajustez l’extent de la boîte : une NavMesh unique suffit ;
 * ce volume ne modifie pas la navigation (contrairement à Nav Modifier Volume).
 */
UCLASS(Blueprintable)
class THEHOUSE_API ATheHouseNPCEjectRegionVolume : public AActor
{
	GENERATED_BODY()

public:
	ATheHouseNPCEjectRegionVolume();

	/** Boîte en espace monde (composant = transform de l’acteur). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|EjectZone")
	TObjectPtr<UBoxComponent> EjectBox;

	/** Distance supplémentaire au-delà de la face de sortie (uu). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|EjectZone", meta = (ClampMin = "0.0"))
	float OutwardPushUU = 120.f;

	UFUNCTION(BlueprintPure, Category = "NPC|EjectZone")
	bool ContainsWorldLocation(const FVector& WorldLocation) const;

	/** Si WorldFromLocation est dans la boîte, calcule un point juste à l’extérieur le long de la face dominante. */
	UFUNCTION(BlueprintCallable, Category = "NPC|EjectZone")
	bool TryComputeEjectionExitWorldLocation(const FVector& WorldFromLocation, FVector& OutWorld) const;
};
