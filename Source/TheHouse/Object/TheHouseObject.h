#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheHouseSelectable.h"
#include "TheHouseObject.generated.h"

class UPrimitiveComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * Slot PNJ : socket sur le mesh + tag d’usage (pour choisir le montage / la logique en Blueprint).
 */
USTRUCT(BlueprintType)
struct FTheHouseNPCInteractionSlot
{
	GENERATED_BODY()

	/** Socket sur le mesh (ex. Seat_0, Dealer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName SocketName;

	/** Tag libre : Sit, Deal, Play, etc. — utilisé par le PNJ / Behavior pour l’animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName PurposeTag;
};

/**
 * Objet casino / décor plaçable.
 * — Zone verte (exclusion) : taille + décalage dans le BP ; les autres objets ne peuvent pas la chevaucher.
 * — Visualiseur : mesh semi-transparent pour voir la zone en déplaçant l’objet.
 * — Surbrillance : Custom Depth pour contour en post-process.
 *
 * Doc projet : Docs/Features/CasinoPlaceableObjects/README.md — index : Docs/README.md — changelog : Docs/Changelog/CHANGELOG.md
 */
UCLASS(Blueprintable)
class THEHOUSE_API ATheHouseObject : public AActor, public ITheHouseSelectable
{
	GENERATED_BODY()

public:
	ATheHouseObject();

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;

public:
	// --- Zone d’exclusion (vert sur ton schéma) : plus grande que le mesh, peut être décalée (ex. place devant) ---
	/** Demi-tailles locales de la zone où aucun autre ATheHouseObject ne peut être posé. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Exclusion zone", meta = (ClampMin = "1.0"))
	FVector PlacementExclusionHalfExtent = FVector(200.f, 160.f, 4.f);

	/**
	 * Décalage du centre de cette zone par rapport à l’origine de l’acteur (local).
	 * Permet de placer la zone « verte » décentrée par rapport au mesh (rouge), comme sur ton dessin.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Exclusion zone")
	FVector PlacementExclusionCenterOffset = FVector::ZeroVector;

	/** Affiche la zone d’exclusion (mesh vert) — typiquement ON pendant drag & drop, OFF une fois posé. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Exclusion zone")
	bool bShowExclusionZonePreview = false;

	/**
	 * Matériau du volume d’aperçu (translucide recommandé). Si null, le mesh reste sans override (à assigner dans le BP).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Exclusion zone")
	UMaterialInterface* ExclusionZonePreviewMaterial = nullptr;

	/** Mesh utilisé pour le volume d’exclusion (par défaut : cube moteur). Remplaçable dans le BP (ex. plan). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Exclusion zone")
	UStaticMesh* ExclusionZonePreviewMesh = nullptr;

	/** Si true, dessine aussi la boîte en debug (shipping : désactivé sauf cette branche). */
	UPROPERTY(EditAnywhere, Category = "Casino|Placement|Debug")
	bool bShowFootprintDebug = false;

	UPROPERTY(EditAnywhere, Category = "Casino|Placement|Debug")
	FColor FootprintDebugColor = FColor::Green;

	/** Boîte locale de la zone d’exclusion (centrée sur PlacementExclusionCenterOffset). */
	UFUNCTION(BlueprintPure, Category = "Casino|Placement")
	FBox GetLocalPlacementExclusionBox() const;

	/** Boîte monde pour la pose actuelle de l’acteur. */
	UFUNCTION(BlueprintPure, Category = "Casino|Placement")
	FBox GetWorldPlacementExclusionBox() const;

	/** Ancien nom — même chose que GetWorldPlacementExclusionBox (compat). */
	UFUNCTION(BlueprintPure, Category = "Casino|Placement", meta = (DeprecatedFunction, DeprecationMessage = "Use GetWorldPlacementExclusionBox"))
	FBox GetWorldFootprintBox() const { return GetWorldPlacementExclusionBox(); }

	UFUNCTION(BlueprintPure, Category = "Casino|Placement", meta = (DeprecatedFunction, DeprecationMessage = "Use GetLocalPlacementExclusionBox"))
	FBox GetLocalFootprintBox() const { return GetLocalPlacementExclusionBox(); }

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	bool TestFootprintOverlapsOthersAt(const FTransform& WorldTransform, AActor* IgnoreActor = nullptr) const;

	/** Met à jour mesh / scale du visualiseur (appelé en construction et quand tu changes les tailles en BP). */
	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	void RefreshExclusionZonePreviewMesh();

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	void SetExclusionZonePreviewVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Casino|Placement")
	bool IsExclusionZonePreviewVisible() const { return bShowExclusionZonePreview; }

	/** Composant du volume vert (pour matériau / tri custom en BP). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Casino|Placement|Exclusion zone")
	UStaticMeshComponent* ExclusionZoneVisualizer = nullptr;

	// --- Surbrillance (contour) ---
	UPROPERTY(EditAnywhere, Category = "Casino|Highlight", meta = (ClampMin = "0", ClampMax = "255"))
	uint8 HighlightStencilValue = 1;

	UFUNCTION(BlueprintCallable, Category = "Casino|Highlight")
	void SetHighlighted(bool bHighlighted);

	UFUNCTION(BlueprintPure, Category = "Casino|Highlight")
	bool IsHighlighted() const { return bHighlighted; }

	// --- PNJ / animations : sockets + tag par slot (montages gérés en BP ou Behavior) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Casino|NPC")
	TArray<FTheHouseNPCInteractionSlot> NPCInteractionSlots;

	// --- ITheHouseSelectable ---
	virtual void OnSelect() override;
	virtual void OnDeselect() override;
	virtual bool IsSelectable() const override;

protected:
	void ApplyHighlightToRenderers(bool bOn);
	void ApplyExclusionZoneMaterial();
	bool ShouldSkipHighlightOnPrimitive(UPrimitiveComponent* Prim) const;

	UPROPERTY(Transient)
	bool bHighlighted = false;
};
