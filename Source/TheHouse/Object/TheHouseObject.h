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
 * — Visualiseur : mesh semi-transparent ; en preview seuls valid/invalid s’appliquent à ce volume, pas au mesh de l’objet.
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

	/** Valeur de revente / remboursement quand on "Vendre" via le menu contextuel.
	 *  Si = 0 : par défaut on utilise PurchasePrice (voir RTS_SellPlacedObject). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Casino|Economy", meta=(ClampMin="0"))
	int32 SellValue = 0;

	/** Coût en argent pour placer une nouvelle instance depuis le catalogue RTS (0 = gratuit). Le stock ne refacture pas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Casino|Economy", meta=(ClampMin="0"))
	int32 PurchasePrice = 0;

	// --- Panneau paramètres (WBP sur le PlayerController : PlacedObjectSettingsWidgetClass) ---
	// Le PC n’ouvre le panneau qu’après sélection trace (ECC_Visibility) ; filtre UMG : TheHouse_IsCursorOverBlockingRTSViewportUI.

	/** Si true : clic gauche RTS sur cet objet ouvre le WBP de paramètres (sinon sélection seulement). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Casino|UI|Parameters Panel")
	bool bOpenParametersPanelOnLeftClick = false;

	/** Si true : l’objet peut faire circuler de l’argent côté PNJ (dépense / retour — affiché dans le WBP). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Casino|UI|Parameters Panel|Economy")
	bool bNpcMoneyFlowEnabled = false;

	/** Somme qu’un PNJ paie pour jouer une fois (ex. mise). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Casino|UI|Parameters Panel|Economy", meta=(ClampMin="0", EditCondition="bNpcMoneyFlowEnabled"))
	int32 NpcSpendToPlay = 0;

	/** Somme max que la maison / le joueur peut récupérer (gain max « rapporté »). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Casino|UI|Parameters Panel|Economy", meta=(ClampMin="0", EditCondition="bNpcMoneyFlowEnabled"))
	int32 NpcMaxReturnToHouse = 0;

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

	/**
	 * Centre et demi-extents (monde) pour dessiner un contour autour du rendu principal :
	 * pour ATheHouseObject = bounds de **ObjectMesh** (sans le visualiseur d’exclusion).
	 */
	static void GetActorOutlineDebugBounds(AActor* Actor, FVector& OutCenter, FVector& OutExtent);

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

	/** Vide la liste des bloqueurs (sans toucher au mesh) quand EvaluatePlacementAt n'est pas appelé (ex. trace invalide). */
	void ClearPlacementBlockerActorCaches();

	/** Retire contours Custom Depth et caches bloqueurs (ex. avant Destroy du preview). */
	void TeardownPlacementBlockerVisualFeedback();

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

	/**
	 * Overlay sur les meshes en sélection RTS. Si null : le C++ utilise un MID teinté « crème »
	 * dérivé de M_Placement_Valid (pas le même rendu que la zone d’exclusion verte).
	 * Tu peux assigner ici un MI dédié (ex. contour blanc cassé / léger fresnel).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Highlight")
	UMaterialInterface* SelectionOverlayMaterial = nullptr;

	/**
	 * Nombre max d'acteurs à surligner (perf mobile / Steam Deck). 0 = désactivé.
	 * Custom Depth utilise **HighlightStencilValue** (comme la sélection) si un post-process d’outline est configuré.
	 */
	/** 0 = utiliser 6 pour l’affichage (les BP à 0 ne voyaient aucun bloqueur). */
	UPROPERTY(EditAnywhere, Category = "Casino|Placement|Blocker highlight", meta = (ClampMin = "0", ClampMax = "32"))
	int32 MaxPlacementBlockerOutlines = 6;

	/**
	 * Matériau **overlay** sur les meshes bloquants (visible sans post-process de contour).
	 * Par défaut = même asset que l’invalid placement si présent dans le projet ; sinon null (rien).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Casino|Placement|Blocker highlight")
	UMaterialInterface* PlacementBlockerOverlayMaterial = nullptr;

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
	UMaterialInterface* ResolveSelectionOverlayMaterial() const;
	UMaterialInterface* ResolveBlockerOverlayMaterial() const;
	void ApplyExclusionZoneMaterial();
	bool ShouldSkipHighlightOnPrimitive(UPrimitiveComponent* Prim) const;
	void ApplyPreviewCollisionAndRendering(bool bEnablePreview);
	void CacheOriginalMaterialsIfNeeded();
	void RestoreOriginalMaterials();
	void ApplyPlacementMaterialToRenderers(UMaterialInterface* MaterialOverride);

	bool TestFootprintOverlapsWorldAtWithBlockerList(
		const FTransform& WorldTransform,
		TArray<TObjectPtr<AActor>>* OutSortedUniqueBlockers,
		int32 MaxBlockersToStore) const;

	void RefreshPlacementBlockerHighlights();
	/** Contour fil (DrawDebugBox) autour des bloqueurs : visible sans post-process ni matériau d’overlay spécial. */
	void DrawPlacementBlockerWireBounds();
	void ClearPlacementBlockerHighlights();
	bool PlacementBlockerHighlightTargetsMatch(const TArray<AActor*>& Desired) const;
	void ApplyBlockerOutlineToActor(AActor* Blocker);

	UPROPERTY(Transient)
	bool bHighlighted = false;

	/** Bloqueurs détectés au dernier EvaluatePlacementAt (autres objets casino). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ATheHouseObject>> PlacementFeedbackObjectBlockers;

	/** Bloqueurs monde (murs, géo) au dernier EvaluatePlacementAt. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> PlacementFeedbackWorldBlockers;

	TArray<TWeakObjectPtr<AActor>> PlacementBlockerHighlightTargets;

	struct FPlacementBlockedPrimRestoreState
	{
		TWeakObjectPtr<UPrimitiveComponent> Primitive;
		bool bHadCustomDepth = false;
		int32 StencilValue = 0;
		bool bTouchedOverlay = false;
		TWeakObjectPtr<UMaterialInterface> PreviousOverlay;
		bool bTouchedOverlayMaxDrawDistance = false;
		float PreviousOverlayMaxDrawDistance = 0.f;
	};

	TArray<FPlacementBlockedPrimRestoreState> PlacementBlockerPrimitiveRestoreStack;

	struct FSelectionOverlayRestore
	{
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> PrevOverlay;
		bool bTouchedOverlayMaxDrawDistance = false;
		float PreviousOverlayMaxDrawDistance = 0.f;
	};

	TArray<FSelectionOverlayRestore> SelectionOverlayRestoreStack;

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

class UMaterialInstanceDynamic;
/** MID crème partagé avec la sélection RTS des objets (`ApplyHighlightToRenderers`). */
THEHOUSE_API UMaterialInstanceDynamic* TheHouse_GetSharedRTSSelectionCreamOverlayMID();
