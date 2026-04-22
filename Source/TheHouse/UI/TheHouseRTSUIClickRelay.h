#pragma once

#include "CoreMinimal.h"
#include "TheHouseObject.h"
#include "UObject/Object.h"
#include "TheHouseRTSUIClickRelay.generated.h"

class ATheHousePlayerController;
class ATheHouseNPCCharacter;

UENUM()
enum class ETheHouseRTSUIClickKind : uint8
{
	Catalog,
	Stored,
	ContextOption,
	/** Option du menu contextuel PNJ (distinct du menu objet). */
	ContextNPCOption,
	/** Ordre : PNJ sélectionnés + cible objet sous le curseur. */
	ContextNPCOrderOnObject,
	/** Ordre : PNJ sélectionnés + cible autre PNJ sous le curseur. */
	ContextNPCOrderOnNPC,
	/** Ordre : PNJ sélectionnés + sol sous le curseur. */
	ContextNPCOrderOnGround,
	/** Palette personnel : prévisualisation / pose d’un PNJ staff (clic sur une ligne du panneau). */
	StaffPalette
};

/**
 * Relais UObject pour brancher UButton::OnClicked (délégué dynamique) vers le PlayerController.
 */
UCLASS()
class THEHOUSE_API UTheHouseRTSUIClickRelay : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	ETheHouseRTSUIClickKind Kind = ETheHouseRTSUIClickKind::Catalog;

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> ContextTarget = nullptr;

	UPROPERTY(Transient)
	FName ContextOptionId;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseNPCCharacter> ContextNPCTarget = nullptr;

	UPROPERTY(Transient)
	TSubclassOf<ATheHouseObject> CatalogClass;

	/** Indice dans ATheHousePlayerController::StoredPlaceableStacks (ligne stock = type + quantité). */
	UPROPERTY(Transient)
	int32 StoredIndex = INDEX_NONE;

	/** Palette PNJ staff : classe à poser + coût (voir `FTheHouseNPCStaffPaletteEntry`). */
	UPROPERTY(Transient)
	TSubclassOf<ATheHouseNPCCharacter> StaffNPCClass;

	UPROPERTY(Transient)
	int32 StaffNPCPaletteHireCost = 0;

	/** Si ≥ 0 : indice dans `ATheHousePlayerController::NPCStaffRosterOffers` ; sinon réglage palette legacy (`StaffNPCClass`). */
	UPROPERTY(Transient)
	int32 StaffRosterOfferIndex = INDEX_NONE;

	UFUNCTION()
	void RelayClicked();
};
