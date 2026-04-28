#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheHouseBuildingPortal.generated.h"

class UBoxComponent;
class ATheHouseNPCCharacter;

/**
 * Portail bâtiment (ex: Casino) : contient une zone de trigger + 2 références de spawn points
 * (extérieur + intérieur) pour téléporter un PNJ qui “entre/sort”.
 *
 * Si le monde cible n’est pas chargé, on peut au choix détruire l’acteur et ne garder que l’état persistant.
 */
UCLASS(BlueprintType, Blueprintable)
class THEHOUSE_API ATheHouseBuildingPortal : public AActor
{
	GENERATED_BODY()

public:
	ATheHouseBuildingPortal();

	/** Identifiant stable du bâtiment (Casino_01, Casino_02…). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Building")
	FName BuildingId = NAME_None;

	/**
	 * Point de spawn/TP extérieur : repère **déjà placé dans le niveau** (`ATheHouseNPCSpawnPoint`, Target Point, etc.).
	 * Ne peut pas être défini dans les « Class Defaults » du Blueprint : uniquement sur l’instance du portail dans la carte (voir EditInstanceOnly).
	 * Si la classe dérive de `ATheHouseNPCSpawnPoint`, la position utilise `GetSpawnWorldLocation()` (rayon aléatoire inclus).
	 */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="NPC|Building", meta=(DisplayName="Exterior Point"))
	AActor* ExteriorPoint = nullptr;

	/** Point de spawn/TP intérieur (même principe qu’Exterior Point). */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="NPC|Building", meta=(DisplayName="Interior Point"))
	AActor* InteriorPoint = nullptr;

	/** Si true : quand un PNJ touche le trigger, on l’envoie vers l’intérieur. Sinon vers l’extérieur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Building")
	bool bDirectionIsEntering = true;

	/** Si false : on détruit le PNJ (état persistant seulement) au lieu de téléporter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Building")
	bool bTeleportIfPossible = true;

	/** Si true : téléporte le joueur **en mode FPS** (`ATheHouseFPSCharacter`) comme les PNJ. Sinon uniquement les PNJ. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="NPC|Building")
	bool bTeleportPlayerCharacter = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Building", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Trigger = nullptr;

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void TeleportOrSimulate(ATheHouseNPCCharacter* NPC, bool bEnter) const;

	void TeleportPlayerCharacterIfNeeded(APawn* Pawn, bool bEnter) const;
};

