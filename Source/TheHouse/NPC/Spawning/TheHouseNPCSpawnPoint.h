#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheHouseNPCSpawnPoint.generated.h"

UENUM(BlueprintType)
enum class ETheHouseNPCSpawnPointUsage : uint8
{
	/** Spawns “immersifs” en ville : doivent éviter d’apparaitre dans/près du champ de vision joueur. */
	CityImmersive UMETA(DisplayName = "City (Immersive)"),

	/** Entrée casino : spawns “simulés” (on ignore les contraintes de visibilité). */
	CasinoEntrance UMETA(DisplayName = "Casino Entrance"),
};

/**
 * Point de spawn PNJ : place-le dans la map et choisis son usage (Ville / Casino).
 * Le sous-système de spawn choisit un point valide selon les règles demandées.
 */
UCLASS(BlueprintType, Blueprintable)
class THEHOUSE_API ATheHouseNPCSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	ATheHouseNPCSpawnPoint();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC|Spawning")
	ETheHouseNPCSpawnPointUsage Usage = ETheHouseNPCSpawnPointUsage::CityImmersive;

	/** Pondération si plusieurs points sont candidats. 1 = normal, 0 = jamais choisi. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC|Spawning", meta=(ClampMin="0.0", UIMin="0.0"))
	float Weight = 1.f;

	/**
	 * Rayon minimal (uu) autour du point pour éviter de spawner “au pixel près” au même endroit.
	 * 0 = exact.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC|Spawning", meta=(ClampMin="0.0", UIMin="0.0"))
	float RandomRadiusUU = 0.f;

	/** Si true, le sous-système projette sur la NavMesh (si possible). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC|Spawning")
	bool bProjectToNav = true;

	/** Position (avec random radius) où spawner. */
	UFUNCTION(BlueprintPure, Category="NPC|Spawning")
	FVector GetSpawnWorldLocation() const;
};

