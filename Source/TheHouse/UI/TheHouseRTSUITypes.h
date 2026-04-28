#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "TheHouseObject.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCStaffArchetype.h"
#include "TheHouseRTSUITypes.generated.h"

class ATheHouseNPCCharacter;

/** Identifiants d’options intégrées (menu contextuel RTS). */
namespace TheHouseRTSContextIds
{
	inline const FName DeleteObject(TEXT("Delete"));
	inline const FName StoreObject(TEXT("Store"));
	/** Déplacer un objet déjà placé (même instance ; slots PNJ conservés). */
	inline const FName RelocateObject(TEXT("Relocate"));
}

/**
 * Identifiants d’options du menu contextuel RTS dédié aux PNJ (ATheHouseNPCCharacter).
 * Étendez la liste via `NPCRTSContextMenuOptionDefs` sur le PlayerController, ou gérez des ids
 * personnalisés dans `HandleNPCRTSContextMenuOption` (BlueprintNativeEvent).
 */
namespace TheHouseNPCContextIds
{
	/** Hook Blueprint / jeu (inspection, stats, dialogue…). */
	inline const FName InspectNPC(TEXT("InspectNPC"));

	/**
	 * Entrée C++ : trace un ATheHouseObject placé sous le curseur et appelle
	 * UTheHouseObjectSlotUserComponent::UseObjectSlot sur le PNJ cible (PurposeTag NAME_None).
	 * Requiert le composant sur le personnage ; sinon log dev uniquement.
	 */
	inline const FName UsePlacedObjectAtCursor(TEXT("UsePlacedObjectAtCursor"));
}

/** Entrée du catalogue de pose (palette RTS). */
USTRUCT(BlueprintType)
struct FTheHousePlaceableCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	TSubclassOf<ATheHouseObject> ObjectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	FText DisplayName;

	/** Optionnel : vignette dans la liste catalogue RTS ; réutilisée pour le stock si la même ObjectClass est au catalogue. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	TObjectPtr<UTexture2D> Thumbnail = nullptr;
};

/** Une ligne du panneau Stock : type d’objet + quantité cumulée. */
USTRUCT(BlueprintType)
struct FTheHouseStoredStack
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|RTS")
	TSubclassOf<ATheHouseObject> ObjectClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|RTS")
	int32 Count = 0;
};

/** Une ligne du panneau Personnel (palette staff) — clic depuis le RTS Main Widget. */
USTRUCT(BlueprintType)
struct FTheHouseNPCStaffPaletteEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Palette")
	TSubclassOf<ATheHouseNPCCharacter> NPCClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Palette")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Palette")
	TObjectPtr<UTexture2D> Thumbnail = nullptr;

	/** Coût d'embauche à la pose (≥ 0). 0 = gratuit. Déduit de l'argent à la confirmation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Palette", meta=(ClampMin="0"))
	int32 HireCost = 0;
};

/** Créneau du « bassin » recrutement : à chaque rafraîchissement, génère `OffersPerRefresh` candidats aléatoires. */
USTRUCT(BlueprintType)
struct FTheHouseNPCStaffPoolSlotDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TSubclassOf<ATheHouseNPCCharacter> CharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TObjectPtr<UTheHouseNPCStaffArchetype> StaffArchetype;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment", meta=(ClampMin="1"))
	int32 OffersPerRefresh = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	float SalaryMultiplierMin = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	float SalaryMultiplierMax = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment", meta=(ClampMin="1", ClampMax="5"))
	int32 StarRatingMin = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment", meta=(ClampMin="1", ClampMax="5"))
	int32 StarRatingMax = 5;

	/** Coût d’embauche ≈ ceil(MonthlySalary * HireCostSalaryMonths). 0 = gratuit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment", meta=(ClampMin="0"))
	float HireCostSalaryMonths = 3.f;

	/**
	 * Noms complets prédéfinis (tirage au hasard par offre). Renseigné sur le **PlayerController** :
	 * tableau `NPC Staff Recruitment Pool` → chaque slot — pas sur le Data Asset Staff.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TArray<FText> RandomDisplayNames;

	/**
	 * Prénoms / noms de famille : à chaque `RefreshNPCStaffRecruitmentOffers`, une paire est tirée au hasard
	 * pour chaque offre (candidat distinct). Utilisé seulement si `RandomDisplayNames` est vide et que les deux listes sont non vides.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TArray<FText> RandomGivenNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TArray<FText> RandomFamilyNames;

	/**
	 * Regroupe les offres dans le panneau Personnel (liste de métiers puis candidats).
	 * Si laissé à None, on utilise `StaffArchetype->StaffRoleId` s’il est défini, sinon `Staff`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	FName StaffCategoryId = NAME_None;

	/** Libellé du métier dans la liste des catégories (sinon le nom de `StaffCategoryId`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	FText StaffCategoryLabel;
};

/** Une ligne du panneau Personnel lorsque le bassin recrutement est actif — stats figées pour la prévisualisation et le spawn final. */
USTRUCT(BlueprintType)
struct FTheHouseNPCStaffRosterOffer
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|RTS|NPC|Recruitment")
	int32 OfferIndexInRoster = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TSubclassOf<ATheHouseNPCCharacter> CharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TObjectPtr<UTheHouseNPCStaffArchetype> StaffArchetype;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	float MonthlySalary = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment", meta=(ClampMin="1", ClampMax="5"))
	int32 StarRating = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	FText DisplayName;

	/** Optionnel : vignette dans la liste des recrues (panneau Personnel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	TObjectPtr<UTexture2D> Thumbnail = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment", meta=(ClampMin="0"))
	int32 HireCost = 0;

	/** Si ≥ 0 et dans les bornes des variants de l’archetype, ce mesh est utilisé (preview = spawn). Sinon tirage aléatoire habituel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|NPC|Recruitment")
	int32 MeshVariantRollIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|RTS|NPC|Recruitment")
	FName StaffCategoryId = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|RTS|NPC|Recruitment")
	FText StaffCategoryLabel;
};

/** Définition d’une option de menu contextuel (modifiable dans le BP du PlayerController). */
USTRUCT(BlueprintType)
struct FTheHouseRTSContextMenuOptionDef
{
	GENERATED_BODY()

	/** Identifiant stable : Delete / Store pour les actions intégrées, ou valeur libre pour HandleRTSContextMenuOption. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	FName OptionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	FText Label;
};
