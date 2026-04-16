#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Containers/Map.h"
#include "TheHouseSelectable.h"
#include "TheHouseObject.generated.h"

class UPrimitiveComponent;
class UMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class APawn;
class UWorld;

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

UENUM(BlueprintType)
enum class EObjectPlacementState : uint8
{
	Valid,
	OverlapsObject,
	OverlapsWorld,
	OverlapsGrid
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

	/** Mesh principal de l'objet (celui que tu règles dans le BP). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Casino|Object", meta=(AllowPrivateAccess="true"))
	UStaticMeshComponent* ObjectMesh = nullptr;

	/** Demi-tailles locales de la zone où aucun autre ATheHouseObject ne peut être posé. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Exclusion zone", meta = (ClampMin = "1.0"))
	FVector PlacementExclusionHalfExtent = FVector(200.f, 160.f, 4.f);

	UPROPERTY(EditAnywhere, Category = "Casino|Placement|Exclusion zone")
	UMaterialInterface* ValidPlacementMaterial;

	UPROPERTY(EditAnywhere, Category = "Casino|Placement|Exclusion zone")
	UMaterialInterface* InvalidPlacementMaterial;

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

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Casino|Placement")
	EObjectPlacementState PlacementState = EObjectPlacementState::Valid;

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	bool EvaluatePlacementAt(const FTransform& WorldTransform);

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	void UpdatePlacementVisual();

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	bool SetPlacementTransform(const FTransform& Transform);

	/** Ancien nom — même chose que GetWorldPlacementExclusionBox (compat). */
	UFUNCTION(BlueprintPure, Category = "Casino|Placement", meta = (DeprecatedFunction, DeprecationMessage = "Use GetWorldPlacementExclusionBox"))
	FBox GetWorldFootprintBox() const { return GetWorldPlacementExclusionBox(); }

	UFUNCTION(BlueprintPure, Category = "Casino|Placement", meta = (DeprecatedFunction, DeprecationMessage = "Use GetLocalPlacementExclusionBox"))
	FBox GetLocalFootprintBox() const { return GetLocalPlacementExclusionBox(); }

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	bool TestFootprintOverlapsOthersAt(const FTransform& WorldTransform, AActor* IgnoreActor = nullptr) const;

	UFUNCTION(BlueprintCallable, Category = "Casino|Placement")
	bool TestFootprintOverlapsWorldAt(const FTransform& WorldTransform) const;

	/** Ignore actor (typically the floor/landscape under cursor) for world overlap checks. */
	void SetPlacementWorldIgnoreActor(AActor* Actor) { PlacementWorldIgnoreActor = Actor; }

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

	/** Tente de réserver un slot libre correspondant au tag. */
	UFUNCTION(BlueprintCallable, Category="Casino|NPC")
	bool TryReserveNPCSlot(APawn* RequestingPawn, FName PurposeTag, int32& OutSlotIndex, FTransform& OutSlotWorldTransform);

	/** Libère un slot (par index). */
	UFUNCTION(BlueprintCallable, Category="Casino|NPC")
	bool ReleaseNPCSlotByIndex(int32 SlotIndex, APawn* ReleasingPawn);

	/** Libère tous les slots occupés par ce pawn. */
	UFUNCTION(BlueprintCallable, Category="Casino|NPC")
	int32 ReleaseAllNPCSlotsForPawn(APawn* ReleasingPawn);

	/** Renvoie le transform monde d'un slot (socket du mesh principal si dispo). */
	UFUNCTION(BlueprintCallable, Category="Casino|NPC")
	bool GetNPCSlotWorldTransform(int32 SlotIndex, FTransform& OutSlotWorldTransform) const;

	/** Occupant actuel du slot (null si libre). */
	UFUNCTION(BlueprintPure, Category="Casino|NPC")
	APawn* GetNPCSlotOccupant(int32 SlotIndex) const;

	// --- ITheHouseSelectable ---
	virtual void OnSelect() override;
	virtual void OnDeselect() override;
	virtual bool IsSelectable() const override;

protected:
	void ApplyHighlightToRenderers(bool bOn);
	void ApplyExclusionZoneMaterial();
	bool ShouldSkipHighlightOnPrimitive(UPrimitiveComponent* Prim) const;
	void ApplyPreviewCollisionAndRendering(bool bEnablePreview);
	void CacheOriginalMaterialsIfNeeded();
	void RestoreOriginalMaterials();
	void ApplyPlacementMaterialToRenderers(UMaterialInterface* MaterialOverride);

	UPROPERTY(Transient)
	bool bHighlighted = false;

public:

	// --- Preview / placement workflow ---
	UFUNCTION(BlueprintCallable, Category="Casino|Placement")
	void SetAsPreview(bool bIsPreview);

	UFUNCTION(BlueprintPure, Category="Casino|Placement")
	bool IsPlacementPreviewActor() const { return bIsPreview; }

	UFUNCTION(BlueprintCallable, Category="Casino|Placement")
	bool IsPlacementValid() const { return bPlacementValid; }

	/** Force un état de placement (ex: invalid grid/surface) + met à jour le visuel. */
	UFUNCTION(BlueprintCallable, Category="Casino|Placement")
	void SetPlacementState(EObjectPlacementState NewState);

	UFUNCTION(BlueprintCallable, Category="Casino|Placement")
	void SetPlacementValid(bool bValid);

	UFUNCTION(BlueprintCallable, Category="Casino|Placement")
	void FinalizePlacement();

	// UFUNCTION(BlueprintCallable, Category="Casino|Placement")
	// bool SetPlacementTransform(const FTransform& Transform);

protected:

	UPROPERTY(VisibleAnywhere, Category="Casino|Placement")
	bool bIsPreview = false;

	UPROPERTY(VisibleAnywhere, Category="Casino|Placement")
	bool bPlacementValid = true;

	// Cache runtime-only (not reflected).
	// Keep it simple: raw pointers, owned elsewhere by the Actor/components.
	TMap<UMeshComponent*, TArray<UMaterialInterface*>> CachedOriginalMaterials;

	/** Runtime-only: actor to ignore during world overlap test (floor under cursor). */
	UPROPERTY(Transient)
	TObjectPtr<AActor> PlacementWorldIgnoreActor = nullptr;

	/** Runtime-only: slot occupants aligned with NPCInteractionSlots. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<APawn>> NPCSlotOccupants;
};
