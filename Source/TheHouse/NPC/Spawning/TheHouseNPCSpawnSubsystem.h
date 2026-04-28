#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TheHouseNPCSpawnPoint.h"
#include "TheHouseNPCSpawnSubsystem.generated.h"

class ATheHouseNPCCharacter;
class APlayerController;

USTRUCT(BlueprintType)
struct FTheHouseCityImmersiveSpawnRules
{
	GENERATED_BODY()

	/** Distance minimale caméra → spawn (uu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Spawning", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinDistanceFromCameraUU = 2500.f;

	/** Distance maximale caméra → spawn (uu). 0 = illimité. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Spawning", meta=(ClampMin="0.0", UIMin="0.0"))
	float MaxDistanceFromCameraUU = 0.f;

	/**
	 * Marge écran (pixels) ajoutée autour du viewport : si le point est dans le viewport élargi,
	 * on le considère “trop proche de la vue” (évite les pop à la rotation de caméra).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Spawning", meta=(ClampMin="0.0", UIMin="0.0"))
	float ScreenMarginPx = 320.f;

	/**
	 * Si true, exige que le point soit occlus par de la géométrie WorldStatic entre la caméra et le point
	 * (renforce l’immersion). Si false, la règle est uniquement “hors écran + distances”.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Spawning")
	bool bRequireOccludedFromCamera = true;

	/** Nombre de tentatives (points) max avant fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Spawning", meta=(ClampMin="1", UIMin="1"))
	int32 MaxCandidateTries = 30;
};

/**
 * Sous-système monde : sélection de points de spawn PNJ + règles de spawn (ville immersif / casino random).
 */
UCLASS()
class THEHOUSE_API UTheHouseNPCSpawnSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Spawns immersifs (ville) : hors vue joueur + marge + (option) occlusion. Retourne les PNJ créés. */
	UFUNCTION(BlueprintCallable, Category="NPC|Spawning")
	TArray<ATheHouseNPCCharacter*> SpawnNPCs_CityImmersive(
		TSubclassOf<ATheHouseNPCCharacter> NPCClass,
		int32 Count,
		APlayerController* ViewController,
		const FTheHouseCityImmersiveSpawnRules& Rules);

	/** Spawns “casino” : aléatoire simple sur points CasinoEntrance, sans contraintes de vue. */
	UFUNCTION(BlueprintCallable, Category="NPC|Spawning")
	TArray<ATheHouseNPCCharacter*> SpawnNPCs_CasinoRandom(
		TSubclassOf<ATheHouseNPCCharacter> NPCClass,
		int32 Count);

	/** Liste des points trouvés dans le monde pour un usage donné. */
	UFUNCTION(BlueprintCallable, Category="NPC|Spawning")
	void GetSpawnPointsByUsage(ETheHouseNPCSpawnPointUsage Usage, TArray<ATheHouseNPCSpawnPoint*>& OutPoints) const;

private:
	void GatherSpawnPointsByUsage(ETheHouseNPCSpawnPointUsage Usage, TArray<ATheHouseNPCSpawnPoint*>& Out) const;

	bool IsWorldLocationTooCloseToView(
		APlayerController* ViewController,
		const FVector& WorldLocation,
		float ScreenMarginPx,
		FVector* OutCamLoc = nullptr) const;

	bool IsOccludedFromCamera(APlayerController* ViewController, const FVector& WorldLocation) const;

	ATheHouseNPCSpawnPoint* PickWeightedRandom(const TArray<ATheHouseNPCSpawnPoint*>& Points) const;

	FVector ResolveSpawnLocationFromPoint(ATheHouseNPCSpawnPoint* Point) const;

	ATheHouseNPCCharacter* SpawnOneNPCAtLocation(
		TSubclassOf<ATheHouseNPCCharacter> NPCClass,
		const FVector& WorldLocation,
		const FRotator& WorldRotation);
};

