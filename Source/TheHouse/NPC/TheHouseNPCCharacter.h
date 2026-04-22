// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TheHouse/Core/TheHouseSelectable.h"
#include "TheHouse/NPC/TheHouseNPCIdentity.h"
#include "TheHouse/UI/TheHouseRTSUITypes.h"
#include "TheHouseNPCCharacter.generated.h"

class UTheHouseNPCArchetype;
class UMaterialInterface;

/**
 * Personnage PNJ : corps simulé (navigation, animations) + état runtime (argent client, etc.).
 * Ne hérite pas du PlayerController : possession par ATheHouseNPCAIController (ou dérivés).
 *
 * Flux typique :
 * 1. Data Asset (Staff / Customer / Special) décrit la “recette”.
 * 2. Cette classe applique l’archetype au BeginPlay et s’enregistre dans UTheHouseNPCSubsystem.
 * 3. Comportement fin (BT, usage d’objet) vit sur le contrôleur IA ou des composants ;
 *    voir UTheHouseObjectSlotUserComponent pour réserver un slot sur un ATheHouseObject.
 */
UCLASS(Blueprintable, BlueprintType)
class THEHOUSE_API ATheHouseNPCCharacter : public ACharacter, public ITheHouseNPCIdentity, public ITheHouseSelectable
{
	GENERATED_BODY()

public:
	ATheHouseNPCCharacter();

	// --- AActor ---
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// --- ITheHouseNPCIdentity ---
	virtual ETheHouseNPCCategory GetNPCCategory_Implementation() const override;
	virtual UTheHouseNPCArchetype* GetNPCArchetype_Implementation() const override;

	// --- ITheHouseSelectable (sélection RTS clic gauche, même pipeline que les objets / FPS) ---
	/** Surbrillance RTS : Custom Depth + overlay crème (comme les objets), squelettes y compris child actors. */
	virtual void OnSelect() override;
	virtual void OnDeselect() override;
	virtual bool IsSelectable() const override;

	/** Si false, le PNJ n’est pas sélectionnable au clic RTS. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	bool bCanBeSelected = true;

	/**
	 * Matériau d’overlay pour la surbrillance RTS (MI / M parent).
	 * Si null : repli = même MID « crème » que les objets casino (parent **M_Placement_Valid** — placement valide, pas la zone d’exclusion).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Selection")
	TObjectPtr<UMaterialInterface> RTSSelectionOverlayMaterial;

	/** Data Asset Staff / Customer / Special. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Identity", meta = (DisplayName = "Archetype PNJ"))
	TObjectPtr<UTheHouseNPCArchetype> NPCArchetype;

	/** Nom affiché (embauche depuis roster / jeu) ; si vide on peut retomber sur le nom d’acteur ou l’archetype en UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Identity")
	FText StaffIdentityDisplayName;

	/**
	 * Si ≥ 0 au moment de `ApplyArchetypeDefaults`, sélectionne ce sous-indice dans `UTheHouseNPCStaffArchetype::SkeletalMeshVariants`.
	 * Sinon comportement précédent (variant aléatoire). Réinitialisé à INDEX_NONE après application pour ne pas figer les respawns futurs.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Staff|Visual")
	int32 StaffMeshVariantRollIndex = INDEX_NONE;

	/** Argent disponible pour un client (mis à jour par le gameplay). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Customer")
	float Wallet = 0.f;

	/**
	 * Salaire mensuel effectif pour le personnel — initialisé depuis DefaultMonthlySalary sur l’archetype Staff ;
	 * modifiable par le joueur (UI). Inchangé pour les non‑staff tant que vous ne branchez pas d’autre logique.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Staff")
	float StaffMonthlySalary = 0.f;

	UFUNCTION(BlueprintCallable, Category = "NPC|Staff")
	void SetStaffMonthlySalary(float NewMonthlySalary);

	/**
	 * Niveau staff 1–5 (hors staff : 0). Initialisé depuis DefaultStarRating sur l’archetype Staff ;
	 * la progression / impact éco est à brancher dans votre gameplay (revenus, qualité de service).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Staff", meta = (ClampMin = "0", ClampMax = "5"))
	int32 StaffStarRating = 0;

	UFUNCTION(BlueprintCallable, Category = "NPC|Staff")
	void SetStaffStarRating(int32 NewStars);

	/**
	 * Courbe linéaire d’exemple : 1★→1.0, 5★→2.0. Utilisez-la comme base pour « plus de revenus si plus d’étoiles »,
	 * ou ignorez‑la et faites votre formule en Blueprint.
	 */
	UFUNCTION(BlueprintPure, Category = "NPC|Staff")
	float GetStaffStarProgressionMultiplier() const;

	/** True si archetype Staff et StaffRoleId == `GUARD` (à renseigner sur le Data Asset staff du garde). */
	UFUNCTION(BlueprintPure, Category = "NPC|Staff")
	bool IsStaffGuardNPC() const;

	/**
	 * Déclenché quand un ordre RTS « attaquer » est donné à ce garde (avant déplacement / tirs scriptés côté AIController).
	 * Surchargez en Blueprint pour sortir une arme, jouer une animation, activer un composant d’arme.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "NPC|Combat|Guard")
	void OnGuardAttackOrderIssued(ATheHouseNPCCharacter* AttackTarget);

	/** Retire de l’argent si le solde suffit (typiquement catégorie Customer). */
	UFUNCTION(BlueprintCallable, Category = "NPC|Customer")
	bool TrySpendFromWallet(float Amount);

	/** Ajoute de l’argent au portefeuille (récompense, distributeur, erreur de partie…). */
	UFUNCTION(BlueprintCallable, Category = "NPC|Customer")
	void AddToWallet(float Amount);

	/** Après spawn : surcharge salaire / étoiles / nom issus du roster ; ne change pas la classe BP. */
	UFUNCTION(BlueprintCallable, Category = "NPC|Staff")
	void ApplyInitialHireFromRosterOffer(const FTheHouseNPCStaffRosterOffer& Offer);

protected:
	/**
	 * Applique les valeurs par défaut depuis NPCArchetype (ex. portefeuille initial).
	 * Appelée au BeginPlay et lorsque l’archetype change dans l’éditeur.
	 * Surclassez la version Blueprint ou la fonction _Implementation en C++ ; appelez toujours
	 * la version parent si vous réimplémenterez en Blueprint (noeud Apply Archetype Defaults).
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "NPC|Advanced")
	void ApplyArchetypeDefaults();

	/** Pile de restauration d’overlay + Custom Depth (voir `RTSSelectionOverlayMaterial`). */
	struct FNPCSelectionOverlayRestore
	{
		TWeakObjectPtr<USkeletalMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> PrevOverlay;
		bool bTouchedOverlayMaxDrawDistance = false;
		float PreviousOverlayMaxDrawDistance = 0.f;
	};

	TArray<FNPCSelectionOverlayRestore> NPCSelectionOverlayRestoreStack;

	void RestoreSelectionOverlayStack();
	void ApplySkeletalSelectionHighlights();

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
