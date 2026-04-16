// // Fill out your copyright notice in the Description page of Project Settings.

// #pragma once

// #include "CoreMinimal.h"
// #include "GameFramework/PlayerController.h"
// #include "Placement/TheHousePlacementTypes.h"
// #include "TheHousePlayerController.generated.h"

// class ATheHouseCameraPawn;
// class ATheHouseFPSCharacter;
// class ATheHouseObject;

// /**
//  * Manages the transition between RTS Camera and FPS Character.
//  */
// UCLASS()
// class THEHOUSE_API ATheHousePlayerController : public APlayerController {
//   GENERATED_BODY()

// public:
//   ATheHousePlayerController(const FObjectInitializer& ObjectInitializer);

//   /** Molette RTS : appelé par UTheHouseGameViewportClient (molette souvent
//    * bloquée avant PlayerInput en Game+UI). */
//   void TheHouse_ApplyWheelZoom(float WheelDelta);

//   void DebugStartPlacement();
//   virtual void SetupInputComponent() override;
//   void ConfirmPlacement();

// protected:
//   virtual void BeginPlay() override;
//   virtual void ReceivedPlayer() override;
//   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
//   virtual void OnPossess(APawn *InPawn) override;
//   virtual void SetupInputComponent() override;
//   virtual void PlayerTick(float DeltaTime) override;
//   void UpdatePlacementPreview();
//   bool ValidatePlacement(const FHitResult& Hit) const;

//   UPROPERTY()
//   EPlacementState PlacementState = EPlacementState::None;

//   UPROPERTY()
//   TSubclassOf<ATheHouseObject> PendingPlacementClass;

//   UPROPERTY()
//   ATheHouseObject* PreviewActor = nullptr;

//   UPROPERTY()
//   EPlacementState PlacementState;

//   UPROPERTY()
//   TSubclassOf<ATheHouseObject> PendingPlacementClass;

// public:
//   /** Switch to RTS Mode */
//   UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
//   void SwitchToRTS();

//   /** Switch to FPS Mode at specific locations */
//   UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
//   void SwitchToFPS(FVector TargetLocation, FRotator TargetRotation);

//   /** Helper to switch to FPS at current cursor/camera location (for testing) */
//   UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
//   void DebugSwitchToFPS();

// protected:
//   /** Reference to the RTS Camera Pawn (persistent) */
//   UPROPERTY()
//   ATheHouseCameraPawn *RTSPawnReference;

//   /** Reference to the current FPS Character (if alive) */
//   UPROPERTY()
//   ATheHouseFPSCharacter *ActiveFPSCharacter;

//   UPROPERTY()
//   bool bIsRTSMode;

//   UPROPERTY()
//   ATheHouseObject* CurrentPreviewObject;

//   UPROPERTY()
//   ATheHouseObject* CurrentPlacementPreview = nullptr;

//   // Permet de choisir via Blueprint si le jeu démarre en vue FPS ou RTS
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Startup")
//   bool bStartInFPSMode = false;

//   /** Class to spawn for FPS mode. Set this in BP to the BP_TheHouseFPSCharacter
//    * if you use one. */
//   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera System")
//   TSubclassOf<ATheHouseFPSCharacter> FPSCharacterClass;

//   // --- Input Functions ---
//   void MoveForward(float Value);
//   void MoveRight(float Value);
//   void Zoom(float Value);
//   void RotateCamera(float Value); // Yaw
//   void LookUp(float Value);       // Pitch

//   // Selection
//   UFUNCTION()
//   void Input_SelectPressed();
//   UFUNCTION()
//   void Input_SelectReleased();

//   // Modifier State
//   bool bIsRotateModifierDown;
//   void OnRotateModifierPressed();
//   void OnRotateModifierReleased();

//   // Selection State
//   bool bIsSelecting;
//   FVector2D SelectionStartPos;

//   /** Entrée déplacée hors de Actor::Tick : un BP « Event Tick » sans Parent
//    * bloque tout le Tick C++. */
//   FTimerHandle TheHouseInputPollTimer;
//   void TheHouse_StartInputPollTimer();
//   void TheHouse_PollInputFrame();

// protected:
//   /** Current Target Zoom Length for interpolation */
//   float TargetZoomLength;

//   /** Mouse Rotation Sensitivity */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
//   float RotationSensitivity = 2.0f;

//   /** Multiplicateur du delta brut molette avant application (voir aussi
//    * RTSMinArmDeltaPerWheelEvent). */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System",
//             meta = (ClampMin = "0.25", UIMin = "0.25"))
//   float RTSWheelZoomMultiplier = 4.f;

//   /** Échelle : variation de TargetArmLength = |Value| * RTSZoomSpeed (uu par «
//    * unité » de Value). */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System",
//             meta = (ClampMin = "50", UIMin = "50"))
//   float RTSZoomSpeed = 260.f;

//   /** Plancher : variation minimale du spring arm (uu) par événement molette —
//    * sans ça, souris haute précision ≈ quelques cm. */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System",
//             meta = (ClampMin = "20", UIMin = "20"))
//   float RTSMinArmDeltaPerWheelEvent = 95.f;

//   /** Quand le spring arm est long, augmente l’effet d’un cran de molette (vue
//    * éloignée). */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
//   bool bRTSZoomScalesWithArmLength = true;

//   /** Référence pour bRTSZoomScalesWithArmLength (uu). */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System",
//             meta = (ClampMin = "100", UIMin = "100"))
//   float RTSZoomArmLengthRef = 1000.f;

//   /** 0 = illimité. Sinon, limite le déplacement du pivot (le long de l’axe
//    * caméra) par cran lors du zoom vers le curseur. */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System",
//             meta = (ClampMin = "0", UIMin = "0"))
//   float RTSCursorZoomMaxPanPerEvent = 400.f;

//   /** 0 = mise à jour instantanée du spring arm. >0 = interpolation (effet zoom
//    * fluide). */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System",
//             meta = (ClampMin = "0", UIMin = "0"))
//   float RTSZoomInterpSpeed = 14.f;

//   /** Log molette + longueurs (Output Log) — utile si le zoom « ne fait rien »
//    * (collision spring arm, etc.). */
//   UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System|Debug")
//   bool bDebugLogRTSZoom = false;

// public:
//   virtual void Tick(float DeltaTime) override;
// };

///

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Placement/TheHousePlacementTypes.h"
#include "UI/TheHouseRTSUITypes.h"
#include "TheHousePlayerController.generated.h"

class ATheHouseCameraPawn;
class ATheHouseFPSCharacter;
class ATheHouseObject;
class UTheHouseRTSMainWidget;
class UTheHouseRTSContextMenuWidget;
class UTheHouseFPSHudWidget;

USTRUCT(BlueprintType)
struct FTheHousePlacementGridSettings
{
	GENERATED_BODY()

	/** Taille d'une cellule (uu). 100 = 1m si ton projet est en cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement|Grid", meta=(ClampMin="1.0"))
	float CellSize = 200.f;

	/** Offset monde pour aligner la grille (ex: centrer sur un point). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement|Grid")
	FVector WorldOrigin = FVector::ZeroVector;

	/** Autorise le placement seulement si la surface est "sol" (normal Z >= threshold). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement|Grid", meta=(ClampMin="0.0", ClampMax="1.0"))
	float MinUpNormalZ = 0.75f;

	/** Si false, la position suit exactement le hit sans snap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Placement|Grid")
	bool bEnableGridSnap = true;
};

/**
 * RTS / FPS hybrid PlayerController
 */
UCLASS()
class THEHOUSE_API ATheHousePlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ATheHousePlayerController(const FObjectInitializer& ObjectInitializer);

    // =========================================================
    // LIFECYCLE
    // =========================================================
    virtual void BeginPlay() override;
    virtual void ReceivedPlayer() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void Tick(float DeltaTime) override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;

    // =========================================================
    // MODE SWITCH (RTS / FPS)
    // =========================================================
    UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
    void SwitchToRTS();

    UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
    void SwitchToFPS(FVector TargetLocation, FRotator TargetRotation);

    UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
    void DebugSwitchToFPS();

    // =========================================================
    // INPUT DEBUG / RTS
    // =========================================================
    void TheHouse_ApplyWheelZoom(float WheelDelta);

    /** Menu contextuel RTS (clic droit) — appelé aussi depuis UTheHouseGameViewportClient en Game+UI. */
    void TryOpenRTSContextMenuAtCursor();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Zoom(float Value);
    void RotateCamera(float Value);
    void LookUp(float Value);

    // =========================================================
    // SELECTION (RTS)
    // =========================================================
    UFUNCTION()
    void Input_SelectPressed();

    UFUNCTION()
    void Input_SelectReleased();

    bool bIsSelecting = false;
    FVector2D SelectionStartPos;

    // =========================================================
    // PLACEMENT SYSTEM (DEBUG / RTS BUILD MODE)
    // =========================================================
    void DebugStartPlacement();
    void ConfirmPlacement();
    void CancelPlacement();

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|Placement")
    void StartPlacementPreviewForClass(TSubclassOf<ATheHouseObject> InClass);

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|Placement")
    void ConsumeStoredPlaceableAndBeginPreview(int32 StoredIndex);

    const TArray<FTheHousePlaceableCatalogEntry>& GetPlaceableCatalog() const { return PlaceableCatalog; }
    const TArray<TSubclassOf<ATheHouseObject>>& GetStoredPlaceables() const { return StoredPlaceables; }

    const TArray<FTheHouseRTSContextMenuOptionDef>& GetRTSContextMenuOptionDefs() const { return RTSContextMenuOptionDefs; }

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
    void ExecuteRTSContextMenuOption(FName OptionId, ATheHouseObject* TargetObject);

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
    void RefreshRTSMainWidget();

protected:

    // =========================================================
    // PLACEMENT INTERNAL STATE
    // =========================================================
    UPROPERTY()
    EPlacementState PlacementState = EPlacementState::None;

    UPROPERTY(EditDefaultsOnly, Category = "Placement")
    TSubclassOf<ATheHouseObject> PendingPlacementClass;

    UPROPERTY()
    ATheHouseObject* PreviewActor = nullptr;

    void SpawnPlacementPreviewFromPendingClass();

    void UpdatePlacementPreview();
    bool ValidatePlacement(const FHitResult& Hit) const;

    // =========================================================
    // GRID PLACEMENT
    // =========================================================
    UPROPERTY(EditDefaultsOnly, Category="Placement|Grid")
    FTheHousePlacementGridSettings PlacementGrid;

    /** Cellules occupées par des objets placés (simple cache runtime). */
    TSet<FIntPoint> OccupiedGridCells;

    // --- Placement rotation (preview) ---
    UPROPERTY(EditDefaultsOnly, Category="Placement|Rotation", meta=(ClampMin="1.0", ClampMax="180.0"))
    float PlacementRotationStepDegrees = 15.f;

    /** Rotation yaw courante du preview (degrés). */
    float PlacementPreviewYawDegrees = 0.f;

    FVector SnapWorldToGridXY(const FVector& WorldLocation) const;
    FIntPoint WorldToGridCell(const FVector& WorldLocation) const;
    void GetFootprintCellsForTransform(const ATheHouseObject* Obj, const FTransform& WorldTransform, TArray<FIntPoint>& OutCells) const;
    bool AreAnyCellsOccupied(const TArray<FIntPoint>& Cells) const;
    void MarkCellsOccupied(const TArray<FIntPoint>& Cells);
    void MarkCellsFree(const TArray<FIntPoint>& Cells);

    // =========================================================
    // CAMERA / PAWN REFERENCES
    // =========================================================
    UPROPERTY()
    ATheHouseCameraPawn* RTSPawnReference;

    UPROPERTY()
    ATheHouseFPSCharacter* ActiveFPSCharacter;

    // =========================================================
    // INPUT STATE
    // =========================================================
    bool bIsRotateModifierDown = false;

    void OnRotateModifierPressed();
    void OnRotateModifierReleased();

    FTimerHandle TheHouseInputPollTimer;
    void TheHouse_StartInputPollTimer();
    void TheHouse_PollInputFrame();

    /** Front montant RMB pour le poll (Game+UI). */
    bool bPrevPollRMBHeld = false;

    /** Anti double ouverture menu même frame (BindAction + poll). */
    double LastRTSContextMenuOpenDebounceSeconds = -1.0;

    void ClampRtsPawnAboveGround(ATheHouseCameraPawn* Cam);

    /** **TheHouseRTSZoomModifier** (défaut Shift) + repli Shift : en preview, molette = pivot ; sinon bloque le zoom molette/+/-. Zoom = molette sans Shift. */
    bool IsRtsZoomMouseModifierHeld() const;

    /** **TheHousePlacementRotateModifier** (défaut Ctrl) + repli Ctrl : en preview, molette = pivot (avec TheHouse_ApplyWheelZoom). */
    bool IsPlacementRotateModifierHeld() const;

    // =========================================================
    // CAMERA SETTINGS
    // =========================================================
    float TargetZoomLength = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RotationSensitivity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RTSWheelZoomMultiplier = 4.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RTSZoomSpeed = 260.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RTSMinArmDeltaPerWheelEvent = 95.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    bool bRTSZoomScalesWithArmLength = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RTSZoomArmLengthRef = 1000.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RTSCursorZoomMaxPanPerEvent = 400.f;

    /** Distance min du spring arm (zoom max). Au-dessous la caméra passe vite sous le sol avec un boom incliné. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "200.0"))
    float RTSCameraMinArmLength = 520.f;

    /** Distance max du spring arm (zoom arrière). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "500.0"))
    float RTSCameraMaxArmLength = 4000.f;

    /** Après le zoom « vers le curseur », le pivot est remonté si une trace trouve le sol en dessous. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "0.0"))
    float RTSPawnMinClearanceAboveGroundUU = 120.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
    float RTSZoomInterpSpeed = 14.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System|Debug")
    bool bDebugLogRTSZoom = false;

    UPROPERTY(EditDefaultsOnly, Category="FPS")
    TSubclassOf<class ATheHouseFPSCharacter> FPSCharacterClass;

    // =========================================================
    // UI (RTS / FPS)
    // =========================================================
    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TSubclassOf<UTheHouseRTSMainWidget> RTSMainWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TSubclassOf<UTheHouseRTSContextMenuWidget> RTSContextMenuWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|FPS")
    TSubclassOf<UTheHouseFPSHudWidget> FPSHudWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TArray<FTheHousePlaceableCatalogEntry> PlaceableCatalog;

    /** Objets retirés de la carte (classes) — réutilisables depuis le panneau Stock. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|UI|RTS")
    TArray<TSubclassOf<ATheHouseObject>> StoredPlaceables;

    /** Options du menu contextuel (clic droit sur un ATheHouseObject placé). Delete / Store sont gérés en C++. */
    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TArray<FTheHouseRTSContextMenuOptionDef> RTSContextMenuOptionDefs;

    UPROPERTY(Transient)
    TObjectPtr<UTheHouseRTSMainWidget> RTSMainWidgetInstance;

    UPROPERTY(Transient)
    TObjectPtr<UTheHouseRTSContextMenuWidget> RTSContextMenuInstance;

    UPROPERTY(Transient)
    TObjectPtr<UTheHouseFPSHudWidget> FPSHudWidgetInstance;

    /** Si différent de INDEX_NONE, une entrée stock sera consommée au placement confirmé. */
    int32 PendingStockConsumeIndex = INDEX_NONE;

    void InitializeModeWidgets();
    void UpdateModeWidgetsVisibility();
    void CloseRTSContextMenu();

    /** Alt physique : rotation RTS ; on n’ouvre pas le menu contextuel sur RMB pendant Alt. */
    bool IsRtsCameraRotateModifierPhysicallyDown() const;

    /** Trace sous le curseur (Visibility + secours WorldDynamic / multi-hit). */
    ATheHouseObject* FindPlacedTheHouseObjectUnderCursor() const;

    UFUNCTION()
    void Input_CancelOrCloseRts();

    UFUNCTION()
    void Input_RTSObjectContext();

    void RTS_DeletePlacedObject(ATheHouseObject* Obj);
    void RTS_StorePlacedObject(ATheHouseObject* Obj);

    void EnsureDefaultRTSContextMenuDefs();

    UFUNCTION(BlueprintNativeEvent, Category="TheHouse|UI|RTS")
    void HandleRTSContextMenuOption(FName OptionId, ATheHouseObject* TargetObject);
};