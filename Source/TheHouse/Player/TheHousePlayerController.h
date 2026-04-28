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
#include "Engine/HitResult.h"
#include "GameFramework/PlayerController.h"
#include "Placement/TheHousePlacementTypes.h"
#include "UI/TheHouseRTSUITypes.h"
#include "UI/TheHousePlacedObjectSettingsWidget.h"
#include "TheHousePlayerController.generated.h"

class ATheHouseCameraPawn;
class ATheHouseFPSCharacter;
class ATheHouseObject;
class ATheHouseNPCCharacter;
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

    /**
     * Clic droit RTS : si au moins un PNJ est sélectionné au gauche — ordres sur objet / autre PNJ (menus dédiés),
     * sinon menu PNJ « inspecter » sur PNJ seul, menu objet sur objet, sinon MoveTo au sol.
     * Le clic droit ne sélectionne pas (la sélection reste au clic gauche).
     */
    UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI")
    void TryOpenRTSContextMenuAtCursor();

    /** Ouvre le menu contextuel pour un objet déjà placé (ex. clic gauche si bOpenContextMenuWhenLeftClickObject). */
    UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI")
    void TryOpenRTSContextMenuForObject(ATheHouseObject* Obj);

    /** Ouvre le menu contextuel PNJ (mêmes garde-fous que le mode RTS / Alt / etc. que l’objet). */
    UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI|NPC")
    void TryOpenRTSContextMenuForNPC(ATheHouseNPCCharacter* Npc);

    /**
     * Échap : ferme en cascade menu contextuel, prévisualisation de placement, rectangle de sélection,
     * sélection RTS, puis appelle EscapeCloseOverlays sur le widget RTS principal (WBP).
     * Retourne true si quelque chose a été annulé / fermé (consommer la touche pour laisser un second Échap au menu pause).
     */
    bool TryConsumeEscapeForRtsUi();

    void MoveForward(float Value);
    void MoveRight(float Value);
    void Zoom(float Value);
    void RotateCamera(float Value);
    void LookUp(float Value);

    /** Actions TheHouseFPS* — touches modifiables dans Project Settings → Input ou via UInputSettings (écran options). */
    void Input_FPSJumpPressed();
    void Input_FPSJumpReleased();
    void Input_FPSRunPressed();
    void Input_FPSRunReleased();
    void Input_FPSCrouchPressed();
    void Input_FPSCrouchReleased();
    void Input_FPSFirePressed();

    // =========================================================
    // SELECTION (RTS)
    // =========================================================
    UFUNCTION()
    void Input_SelectPressed();

    UFUNCTION()
    void Input_SelectReleased();

    /** Double clic gauche RTS : ouvre le panneau paramètres de l’objet sous le curseur (si activé sur l’objet). */
    UFUNCTION()
    void Input_SelectDoubleClick();

    bool bIsSelecting = false;
    /** True seulement si Input_SelectPressed a démarré un drag HUD depuis le monde (pas un clic pur sur l’UMG). */
    bool bRtsSelectGestureFromWorld = false;
    FVector2D SelectionStartPos;

    /** Acteurs actuellement « sélectionnés » (surbrillance RTS) — pour tout désélectionner avant une nouvelle sélection. */
    TArray<TWeakObjectPtr<AActor>> RTSSelectedActors;

    /** Si false : ne ferme pas le panneau paramètres objet (RTS_SelectExclusive appelle ainsi avant OpenPlacedObjectSettingsWidgetFor pour éviter destroy/recreate au même clic). Défaut : comportement précédent (fermeture). */
    void RTS_DeselectAll(bool bClosePlacedObjectSettings = true);
    void RTS_SelectExclusive(AActor* Actor);
    void RTS_SelectFromBox(const TArray<AActor*>& CandidateActors);

    // =========================================================
    // PLACEMENT SYSTEM (DEBUG / RTS BUILD MODE)
    // =========================================================
    void DebugStartPlacement();
    void ConfirmPlacement();
    void CancelPlacement();

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|Placement")
    void StartPlacementPreviewForClass(TSubclassOf<ATheHouseObject> InClass);

    /** @param StoredStackIndex indice dans GetStoredPlaceableStacks() (une ligne = un type + quantité). */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|Placement")
    void ConsumeStoredPlaceableAndBeginPreview(int32 StoredStackIndex);

    /** Repositionne l’instance déjà sur la carte (annulation = retour à la pose initiale). Les PNJ assignés à cet objet restent sur la même instance. */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|Placement")
    void StartRelocationPreviewForPlacedObject(ATheHouseObject* PlacedObject);

    const TArray<FTheHousePlaceableCatalogEntry>& GetPlaceableCatalog() const { return PlaceableCatalog; }

    /** Stock par type (quantités). Accessible aussi en BP via la propriété StoredPlaceableStacks. */
    const TArray<FTheHouseStoredStack>& GetStoredPlaceableStacks() const { return StoredPlaceableStacks; }

	/** Remplace le stock (Load / debug). Rafraîchit l’UI RTS. */
	UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|Stock")
	void SetStoredPlaceableStacks(const TArray<FTheHouseStoredStack>& NewStacks);

    const TArray<FTheHouseRTSContextMenuOptionDef>& GetRTSContextMenuOptionDefs() const { return RTSContextMenuOptionDefs; }

    /** Lignes du menu contextuel PNJ (WBP dérivé de UTheHouseNPCRTSContextMenuUMGWidget). */
    const TArray<FTheHouseRTSContextMenuOptionDef>& GetNPCRTSContextMenuOptionDefs() const { return NPCRTSContextMenuOptionDefs; }

    /** Rempli à l’ouverture du menu « ordre sur objet » (voir GatherNPCOrderActionsOnObject). */
    const TArray<FTheHouseRTSContextMenuOptionDef>& GetNPCOrderOnObjectRuntimeOptionDefs() const { return NPCOrderOnObjectRuntimeOptionDefs; }

    /** Rempli à l’ouverture du menu « ordre sur autre PNJ » (voir GatherNPCOrderActionsOnNPC). */
    const TArray<FTheHouseRTSContextMenuOptionDef>& GetNPCOrderOnNPCRuntimeOptionDefs() const { return NPCOrderOnNPCRuntimeOptionDefs; }

    /** Rempli à l’ouverture du menu « ordre sur sol » (voir GatherNPCOrderActionsOnGround). */
    const TArray<FTheHouseRTSContextMenuOptionDef>& GetNPCOrderOnGroundRuntimeOptionDefs() const { return NPCOrderOnGroundRuntimeOptionDefs; }

    /** Au moins un ATheHouseNPCCharacter sélectionnable dans RTSSelectedActors (sélection clic gauche). */
    UFUNCTION(BlueprintPure, Category = "TheHouse|RTS|UI|NPC|Orders")
    bool HasSelectedNPCsForRTSOrders() const;

    /** Copie les PNJ sélectionnés (ordre = RTSSelectedActors). */
    UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI|NPC|Orders")
    void GetSelectedNPCsForRTSOrders(TArray<ATheHouseNPCCharacter*>& OutNPCs) const;

    /** Arrête tir / focus combat scripté pour les PNJ sélectionnés dont l’AIController est ATheHouseNPCAIController (sans désélectionner). */
    UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|NPC|Combat")
    void CancelScriptedGuardAttackOnSelectedNPCs();

    // =========================================================
    // ECONOMY
    // =========================================================
    UFUNCTION(BlueprintPure, Category="TheHouse|Economy")
    int32 GetMoney() const { return Money; }

    UFUNCTION(BlueprintCallable, Category="TheHouse|Economy")
    void AddMoney(int32 Delta);

    /** Fixe l’argent du joueur (≥ 0) et rafraîchit l’UI RTS. */
    UFUNCTION(BlueprintCallable, Category="TheHouse|Economy")
    void SetMoney(int32 NewMoney);

    // =========================================================
    // IN-GAME TIME (DAY/NIGHT) + PAYROLL
    // =========================================================
    /** Horloge in-game : 1 journée (24h) correspond à cette durée IRL (secondes). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Time", meta=(ClampMin="1.0", UIMin="1.0"))
    float InGameDayLengthRealSeconds = 900.f; // 15 min = 1 journée

    /** Heures par jour (fixe). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Time", meta=(ClampMin="1.0", ClampMax="48.0"))
    float InGameHoursPerDay = 24.f;

    /** Jours par “mois” pour convertir un salaire mensuel en salaire journalier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Economy|Payroll", meta=(ClampMin="1", ClampMax="60"))
    int32 PayrollDaysPerMonth = 30;

    /** Si true, prélève automatiquement les salaires des employés une fois par jour in-game (après 24h de travail). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Economy|Payroll")
    bool bEnableDailyStaffPayroll = true;

    /** Affichage : texte “Jour X · HH:MM”. */
    UFUNCTION(BlueprintPure, Category="TheHouse|Time")
    FText GetInGameClockText() const;

    /** Affichage : progression [0..1) dans la journée. */
    UFUNCTION(BlueprintPure, Category="TheHouse|Time")
    float GetInGameDayProgress01() const;

    UFUNCTION(BlueprintPure, Category="TheHouse|Time")
    int32 GetInGameDayIndex() const { return InGameDayIndex; }

	/** Temps in-game absolu (secondes) depuis le début de la simulation (peut dépasser 1 jour). */
	UFUNCTION(BlueprintPure, Category="TheHouse|Time")
	float GetInGameTimeSeconds() const { return InGameTimeSeconds; }

	/**
	 * Restaure l’horloge in-game (pour Load). Redémarre le timer si nécessaire.
	 * @param NewInGameTimeSeconds secondes in-game (>=0)
	 * @param NewDayIndex jour courant (>=0) ; si incohérent avec NewInGameTimeSeconds, l’index sera recalculé.
	 */
	UFUNCTION(BlueprintCallable, Category="TheHouse|Time")
	void SetInGameClockState(float NewInGameTimeSeconds, int32 NewDayIndex);

    /** Prix catalogue : lu sur le CDO de la classe d’objet (PurchasePrice). */
    UFUNCTION(BlueprintPure, Category="TheHouse|Economy")
    int32 GetPurchasePriceForClass(TSubclassOf<ATheHouseObject> ObjectClass) const;

    /** Suffisamment d’argent pour une pose depuis le catalogue (hors stock / hors déplacement). */
    UFUNCTION(BlueprintPure, Category="TheHouse|Economy")
    bool CanAffordCatalogPurchaseForClass(TSubclassOf<ATheHouseObject> ObjectClass) const;

    /** Argent de départ / en cours : visible dans Class Defaults du BP qui hérite de ce PlayerController. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Economy", meta=(ClampMin="0"))
    int32 Money = 1000;

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
    void ExecuteRTSContextMenuOption(FName OptionId, ATheHouseObject* TargetObject);

    /** Exécution d’une option du menu contextuel PNJ (bouton UMG → relais). Voir TheHouseNPCContextIds. */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI|NPC")
    void ExecuteNPCRTSContextMenuOption(FName OptionId, ATheHouseNPCCharacter* TargetNPC);

    /** Remplit OutOptionDefs (déjà vidé par l’appelant) pour le menu ordre sur objet ; surcharger en BP sur le PlayerController. */
    UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|RTS|UI|NPC|Orders")
    void GatherNPCOrderActionsOnObject(ATheHouseObject* TargetObject, UPARAM(ref) TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs);

    UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|RTS|UI|NPC|Orders")
    void GatherNPCOrderActionsOnNPC(ATheHouseNPCCharacter* TargetNPC, UPARAM(ref) TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs);

    UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|RTS|UI|NPC|Orders")
    void GatherNPCOrderActionsOnGround(TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs);

    UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|RTS|UI|NPC|Orders")
    void ExecuteNPCOrderOnObjectAction(FName OptionId, ATheHouseObject* TargetObject);

    UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|RTS|UI|NPC|Orders")
    void ExecuteNPCOrderOnNPCAction(FName OptionId, ATheHouseNPCCharacter* TargetNPC);

    /** Exécute une action “sol” sur le dernier point de sol détecté au clic droit (voir GetLastNPCOrderGroundTarget). */
    UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|RTS|UI|NPC|Orders")
    void ExecuteNPCOrderOnGroundAction(FName OptionId);

    UFUNCTION(BlueprintPure, Category = "TheHouse|RTS|UI|NPC|Orders")
    FVector GetLastNPCOrderGroundTarget() const { return LastNPCOrderGroundTarget; }

    /** Appeler depuis le menu ordres PNJ (ex. relay UMG) avant ExecuteNPCOrder* : le relâchement LMB qui suit ne doit pas vider la sélection RTS. */
    UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI|NPC|Orders")
    void NotifyRtsOrderMenuOptionClickedFromUi();

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
    void RefreshRTSMainWidget();

    /** Ferme le WBP de paramètres d’objet casino (bouton Fermer dans le WBP). */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
    void ClosePlacedObjectSettingsPanel();

    /** Accès lecture seule pour un WBP de panneau Personnel (RTS Main Widget : hors « protected » pour les widgets UObject). */
    UFUNCTION(BlueprintPure, Category="TheHouse|RTS|NPC|Palette")
    const TArray<FTheHouseNPCStaffPaletteEntry>& GetNPCStaffPalette() const { return NPCStaffPalette; }

    UFUNCTION(BlueprintPure, Category="TheHouse|RTS|NPC|Recruitment")
    const TArray<FTheHouseNPCStaffRosterOffer>& GetNPCStaffRosterOffers() const { return NPCStaffRosterOffers; }

    /** Régénère les candidats à l’embauche (appel automatique au BeginPlay si le bassin n’est pas vide). */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Recruitment")
    void RefreshNPCStaffRecruitmentOffers();

    /** Vide le bassin (règles) puis regénère roster + UI. */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Recruitment")
    void ClearNPCStaffRecruitmentPoolSlots();

    /**
     * Ajoute une règle au bassin. Si `bRebuildRosterAndUi` est false, enchaîne plusieurs ajouts puis appelle une fois `RefreshNPCStaffRecruitmentOffers`.
     */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Recruitment", meta=(AdvancedDisplay="bRebuildRosterAndUi"))
    void AddNPCStaffRecruitmentPoolSlot(const FTheHouseNPCStaffPoolSlotDef& Slot, bool bRebuildRosterAndUi = true);

    /** Démarre la prévisualisation de pose d’un staff (annule la prévisualisation d’objet si besoin). */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Palette")
    void StartStaffNpcPlacementFromPalette(TSubclassOf<ATheHouseNPCCharacter> NPCClass, int32 HireCost = 0);

    /** Prévisualisation à partir d’une ligne du roster (`NPCStaffRosterOffers`). */
    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Recruitment")
    void StartStaffNpcPlacementFromRosterOffer(int32 RosterOfferIndex);

    /** Panneau recrutement : None = liste des métiers ; sinon filtre sur `FTheHouseNPCStaffRosterOffer::StaffCategoryId`. Réinitialisé au refresh du roster. */
    UFUNCTION(BlueprintPure, Category="TheHouse|RTS|NPC|Recruitment")
    FName GetStaffUiRosterBrowseCategoryId() const { return StaffUiRosterBrowseCategoryId; }

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Recruitment")
    void StaffUiSetRosterCategoryFilter(FName CategoryId);

    UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|NPC|Recruitment")
    void StaffUiClearRosterCategoryFilter();

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

    /** Offre staff en cours (palette legacy ou ligne roster) — utilisée au confirm pour classe, coût et stats. */
    UPROPERTY()
    FTheHouseNPCStaffRosterOffer PendingStaffSpawnOffer;

	/** Si >= 0 : l’offre vient de `NPCStaffRosterOffers[PendingStaffSpawnOfferRosterIndex]` (consommation au confirm). */
	UPROPERTY()
	int32 PendingStaffSpawnOfferRosterIndex = INDEX_NONE;

    UPROPERTY()
    ATheHouseNPCCharacter* StaffPlacementPreviewNpc = nullptr;

    bool bStaffNpcPlacementLocationValid = false;

    /** Réentrance / double-chemin même frame — évite deux spawns live sur un seul clic. */
    bool bStaffNpcConfirmInFlight = false;

    void ConfigureStaffNpcDeferredFromOffer(ATheHouseNPCCharacter* Npc, const FTheHouseNPCStaffRosterOffer& Offer) const;

    void SpawnPlacementPreviewFromPendingClass();

    void UpdatePlacementPreview();
    void UpdateStaffNpcPlacementPreview();
    bool ValidatePlacement(const FHitResult& Hit) const;

    void ConfirmStaffNpcPlacement();

	// =========================================================
	// RECRUITMENT (ROSTER) RUNTIME
	// =========================================================
	void TheHouse_StartRecruitmentAutoRefreshTimerIfNeeded();
	void TheHouse_OnRecruitmentAutoRefreshTimer();
	FTimerHandle TheHouseRecruitmentRefreshTimer;

	// =========================================================
	// IN-GAME TIME + PAYROLL (RUNTIME)
	// =========================================================
	void TheHouse_StartInGameClockTimerIfNeeded();
	void TheHouse_OnInGameClockTick();
	void TheHouse_HandleNewInGameDay();
	FTimerHandle TheHouseInGameClockTimer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Time")
	float InGameTimeSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Time")
	int32 InGameDayIndex = 0;

	UPROPERTY()
	int32 InGameLastProcessedDayIndex = 0;

    /** Au moins un acteur dans RTSSelectedActors différent de Exclude (pour menus ordres sur PNJ). */
    bool HasRTSSelectionActorOtherThan(const AActor* Exclude) const;

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

    void AddOrIncrementStoredStock(TSubclassOf<ATheHouseObject> ClassToStore);

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

    /** Front LMB (RTS) : même raison que RMB — PlayerInput / BindAction peuvent ne pas voir le clic en Game+UI. */
    bool bPrevPollLMBHeld = false;

    /** Anti double press/release : poll 120 Hz + BindAction « Select » sur le même front (aligné sur `GFrameCounter` UE 5.7+ = uint64). */
    static constexpr uint64 TheHouse_InvalidRtsLmbFrame = 0xFFFFFFFFFFFFFFFFull;
    uint64 LastRtsLmbSelectPressFrame = TheHouse_InvalidRtsLmbFrame;
    uint64 LastRtsLmbSelectReleaseFrame = TheHouse_InvalidRtsLmbFrame;

    /** Après un choix dans le menu ordres (UMG), le relâchement LMB peut arriver une fois le menu fermé : sans cela, Input_SelectReleased appelle RTS_DeselectAll(). */
    bool bRtsSuppressNextSelectReleaseWorldPick = false;

    /** Le timer de poll peut tourner >1× dans le même « instant » monde : éviter double toggle (équivalent sans GFrameCounter). */
    double LastRTSContextMenuRMBHandledTimeSeconds = -1.0;

    /** Dernière ouverture réussie du menu contextuel (écart minimal entre deux TryOpen). */
    double LastRTSContextMenuTryOpenTimeSeconds = -1.0;

    /** Dernier relâchement gauche RTS sur un objet avec panneau paramètres (voir `Input_SelectReleased`, repli double-clic). */
    double LastRtsParamPanelObjectReleaseTimeSeconds = -1.0;
    TWeakObjectPtr<ATheHouseObject> LastRtsParamPanelObjectRelease;

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

    /** Subclass to spawn when entering FPS (e.g. BP child with skeletal mesh + AnimBP). Capsule-only world collision is applied on the pawn unless disabled on the character. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FPS", meta=(DisplayName="FPS Character Class"))
    TSubclassOf<class ATheHouseFPSCharacter> FPSCharacterClass;

    // =========================================================
    // UI (RTS / FPS)
    // =========================================================
    /** WBP dont le parent est TheHouse RTS Main Widget (avec MoneyText / CatalogScroll / StoredScroll), ou laisser vide pour le layout C++ par défaut. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS")
    TSubclassOf<UTheHouseRTSMainWidget> RTSMainWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TSubclassOf<UUserWidget> RTSContextMenuWidgetClass;

    /** WBP hérité de UTheHouseNPCRTSContextMenuUMGWidget (laisser vide = classe C++ par défaut en InitializeModeWidgets). */
    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS|NPC", meta=(DisplayName="Menu contextuel PNJ (classe)"))
    TSubclassOf<UUserWidget> NPCRTSContextMenuWidgetClass;

    /** WBP hérité de UTheHouseNPCOrderContextMenuUMGWidget : menu ordres (PNJ sélectionnés + clic droit sur objet / autre PNJ). */
    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS|NPC|Orders", meta=(DisplayName="Menu ordres PNJ (classe)"))
    TSubclassOf<UUserWidget> NPCOrderContextMenuWidgetClass;

    /** WBP hérité de UTheHousePlacedObjectSettingsWidget : ouvert seulement si l’objet a bOpenParametersPanelOnLeftClick. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS")
    TSubclassOf<UTheHousePlacedObjectSettingsWidget> PlacedObjectSettingsWidgetClass;

    /** Si true, un clic gauche qui sélectionne un objet ouvre aussi le menu contextuel (en plus du panneau de config). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS")
    bool bOpenContextMenuWhenLeftClickObject = false;

    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|FPS")
    TSubclassOf<UTheHouseFPSHudWidget> FPSHudWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TArray<FTheHousePlaceableCatalogEntry> PlaceableCatalog;

    /** Palette PNJ staff (panneau Personnel du RTS Main Widget) : clic = prévisualisation puis clic gauche pour poser.
     * Utilisée seulement si `NPCStaffRosterOffers` est vide après `RefreshNPCStaffRecruitmentOffers`. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS|NPC|Palette")
    TArray<FTheHouseNPCStaffPaletteEntry> NPCStaffPalette;

    /**
     * Bassin = **règles** (types de postes, archetypes, plages salaire, combien d’offres par refresh).
     * Ce n’est pas la liste affichée : celle-ci est `NPCStaffRosterOffers`, regénérée aléatoirement par `RefreshNPCStaffRecruitmentOffers`.
     * Tu peux modifier ce tableau à l’exécution (BP/C++) puis appeler `RefreshNPCStaffRecruitmentOffers` sur un timer ou un événement.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS|NPC|Recruitment")
    TArray<FTheHouseNPCStaffPoolSlotDef> NPCStaffRecruitmentPool;

	/** Si true, un timer régénère automatiquement le roster à intervalle fixe (recrutement “vivant”). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS|NPC|Recruitment")
	bool bAutoRefreshNPCStaffRecruitmentRoster = false;

	/** Intervalle (secondes) entre deux refresh auto. 0 = désactivé. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|UI|RTS|NPC|Recruitment", meta=(ClampMin="0.0", UIMin="0.0"))
	float NPCStaffRecruitmentAutoRefreshIntervalSeconds = 60.f;

    /** Offres courantes affichées dans le panneau Personnel (prioritaire sur la palette statique). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|UI|RTS|NPC|Recruitment")
    TArray<FTheHouseNPCStaffRosterOffer> NPCStaffRosterOffers;

    /** État UI du panneau recrutement (navigation par métier). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="TheHouse|UI|RTS|NPC|Recruitment")
    FName StaffUiRosterBrowseCategoryId = NAME_None;

    /** Stock par type : quantité décrémentée quand un objet issu du stock est replacé et confirmé. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|UI|RTS")
    TArray<FTheHouseStoredStack> StoredPlaceableStacks;

    /** Options du menu contextuel (clic droit sur un ATheHouseObject placé). Delete / Store / Relocate sont gérés en C++.
     * Tableau vide dans un BP enfant : à l’exécution on ajoute Vendre, Mettre en stock, puis Déplacer (Relocate) si absent
     * via EnsureDefaultRTSContextMenuDefs (BeginPlay + ouverture du menu). */
    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS")
    TArray<FTheHouseRTSContextMenuOptionDef> RTSContextMenuOptionDefs;

    /**
     * Options affichées par le menu PNJ. Ids stables (TheHouseNPCContextIds) + libellés localisables.
     * Si le tableau BP est vide au runtime, EnsureDefaultNPCRTSContextMenuDefs ajoute InspectNPC et UsePlacedObjectAtCursor.
     */
    UPROPERTY(EditDefaultsOnly, Category="TheHouse|UI|RTS|NPC")
    TArray<FTheHouseRTSContextMenuOptionDef> NPCRTSContextMenuOptionDefs;

    /** Rempli à chaque ouverture du menu ordre sur objet (GatherNPCOrderActionsOnObject). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="TheHouse|UI|RTS|NPC|Orders")
    TArray<FTheHouseRTSContextMenuOptionDef> NPCOrderOnObjectRuntimeOptionDefs;

    /** Rempli à chaque ouverture du menu ordre sur autre PNJ (GatherNPCOrderActionsOnNPC). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="TheHouse|UI|RTS|NPC|Orders")
    TArray<FTheHouseRTSContextMenuOptionDef> NPCOrderOnNPCRuntimeOptionDefs;

    /** Rempli à chaque ouverture du menu ordre sur sol (GatherNPCOrderActionsOnGround). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="TheHouse|UI|RTS|NPC|Orders")
    TArray<FTheHouseRTSContextMenuOptionDef> NPCOrderOnGroundRuntimeOptionDefs;

    /** Dernier point sol capturé au clic droit (utilisé par ExecuteNPCOrderOnGroundAction). */
    UPROPERTY(Transient, BlueprintReadOnly, Category="TheHouse|UI|RTS|NPC|Orders")
    FVector LastNPCOrderGroundTarget = FVector::ZeroVector;

    UPROPERTY(Transient)
    TObjectPtr<UTheHouseRTSMainWidget> RTSMainWidgetInstance;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> RTSContextMenuInstance;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> RTSNPCContextMenuInstance;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> RTSNPCOrderOnObjectMenuInstance;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> RTSNPCOrderOnNPCMenuInstance;

    UPROPERTY(Transient)
    TObjectPtr<UUserWidget> RTSNPCOrderOnGroundMenuInstance;

    bool AnyRTSContextMenuOpen() const
    {
        return RTSContextMenuInstance != nullptr || RTSNPCContextMenuInstance != nullptr
            || RTSNPCOrderOnObjectMenuInstance != nullptr || RTSNPCOrderOnNPCMenuInstance != nullptr
            || RTSNPCOrderOnGroundMenuInstance != nullptr;
    }

    UPROPERTY(Transient)
    TObjectPtr<UTheHouseFPSHudWidget> FPSHudWidgetInstance;

    UPROPERTY(Transient)
    TObjectPtr<UTheHousePlacedObjectSettingsWidget> PlacedObjectSettingsWidgetInstance = nullptr;

    /** Dernière cible liée au panneau paramètres — permet de réutiliser le même WBP même si un BP vidait la cible dans le widget entre deux clics. */
    TWeakObjectPtr<ATheHouseObject> PlacedObjectSettingsLastBoundObject;

    /** Si différent de INDEX_NONE, indice dans StoredPlaceableStacks : une unité sera retirée au placement confirmé. */
    int32 PendingStockConsumeIndex = INDEX_NONE;

    /** Preview = déplacement d’un objet déjà placé (ne pas Destroy au Cancel). */
    bool bRelocationPreviewActive = false;

    /** Pose sauvegardée avant déplacement (restaurée si Annuler). */
    FTransform RelocationRestoreTransform;

    void InitializeModeWidgets();
    void UpdateModeWidgetsVisibility();
    /** Ferme les menus contextuels RTS (objet, PNJ, ordres) et restaure l’input jeu si besoin. */
    void CloseRTSContextMenu();
    void ClosePlacedObjectSettingsWidget();
    void OpenPlacedObjectSettingsWidgetFor(ATheHouseObject* Obj);

	/** Ferme uniquement les UI modales RTS (menus contextuels + panneau paramètres). */
	void CloseRtsModalUi(bool bClosePlacedObjectSettings = true);

    FVector2D TheHouse_GetContextMenuViewportPosition() const;
    void TheHouse_OpenRTSContextMenuShared(ATheHouseObject* Obj, const FVector2D& MenuScreenPos);
    void TheHouse_OpenRTSContextMenuForNPCShared(ATheHouseNPCCharacter* Npc, const FVector2D& MenuScreenPos);
    void TheHouse_OpenNPCOrderOnObjectMenuShared(ATheHouseObject* Obj, const FVector2D& MenuScreenPos);
    void TheHouse_OpenNPCOrderOnNPCMenuShared(ATheHouseNPCCharacter* Npc, const FVector2D& MenuScreenPos);
    void TheHouse_OpenNPCOrderOnGroundMenuShared(const FVector& GroundLocation, const FVector2D& MenuScreenPos);

    bool IsActorInRTSSelection(AActor* Actor) const;

    /** Ordre MoveTo (NavMesh) pour chaque PNJ actuellement dans RTSSelectedActors. */
    void IssueMoveOrdersToSelectedNPCsAtLocation(const FVector& GoalLocation);

    /**
     * True si le curseur est sur de l’UMG qui doit absorber le clic (ne pas démarrer le drag HUD ni tracer la sélection monde).
     * Menu contextuel / panneau paramètres : test géométrie (fenêtre). Panneau RTS principal : chemin Slate + hit-test
     * ESlateVisibility::Visible sur la chaîne UMG (évite un root WBP plein écran qui bloquait tout via IsUnderLocation).
     */
    bool TheHouse_IsCursorOverBlockingRTSViewportUI() const;

    /**
     * Fenêtres modales uniquement : menus contextuels objet/PNJ/ordres + panneau paramètres objet.
     * À utiliser pour la sélection/désélection monde : sinon le HUD RTS principal marque souvent tout l’écran comme « bloquant »
     * et aucune trace monde ne s’exécute (voir Input_SelectPressed / Released).
     */
    bool TheHouse_IsModalRtsUiBlockingWorldSelection() const;

    /** Alt physique : rotation RTS ; on n’ouvre pas le menu contextuel sur RMB pendant Alt. */
    bool IsRtsCameraRotateModifierPhysicallyDown() const;

    /** Trace sous le curseur (Visibility + secours WorldDynamic / multi-hit). */
    ATheHouseObject* FindPlacedTheHouseObjectUnderCursor() const;

    UFUNCTION()
    void Input_CancelOrCloseRts();

    UFUNCTION()
    void Input_RTSObjectContext();

    void RTS_SellPlacedObject(ATheHouseObject* Obj);
    void RTS_StorePlacedObject(ATheHouseObject* Obj);

    void EnsureDefaultRTSContextMenuDefs();
    void EnsureDefaultNPCRTSContextMenuDefs();

    /** Fusionne les traces souris (cohérence PNJ vs objet vs sol) — une entrée par acteur, distance minimale. */
    void CollectMergedRTSLineHitsUnderCursor(TMap<AActor*, FHitResult>& OutBestHitPerActor) const;

    /** Sol « marchable » depuis les hits fusionnés, sinon trace WorldStatic simple. */
    bool TryResolveGroundClickLocation(const TMap<AActor*, FHitResult>& MergedHits, FVector& OutLocation) const;

    /**
     * Clic simple : acteur ITheHouseSelectable le plus proche le long du rayon, via les mêmes traces
     * que le menu contextuel (sinon Visibility seul ne touche souvent pas le profil Pawn / le mesh).
     */
    AActor* FindPrimarySelectableActorUnderCursorRTS() const;

    /** Après GetActorsInSelectionRectangle, ajoute les PNJ manquants (test AABB écran du mesh). */
    void AppendSelectableNPCsIntersectingRTSBox(const FVector2D& RectCornerA, const FVector2D& RectCornerB, TArray<AActor*>& InOutActors) const;

    UFUNCTION(BlueprintNativeEvent, Category="TheHouse|UI|RTS")
    void HandleRTSContextMenuOption(FName OptionId, ATheHouseObject* TargetObject);

    /**
     * Tous les OptionId du menu PNJ non gérés en C++ dans ExecuteNPCRTSContextMenuOption
     * (ex. InspectNPC, règles métier) : surclassez en Blueprint sur le PlayerController.
     */
    UFUNCTION(BlueprintNativeEvent, Category="TheHouse|UI|RTS|NPC")
    void HandleNPCRTSContextMenuOption(FName OptionId, ATheHouseNPCCharacter* TargetNPC);
};