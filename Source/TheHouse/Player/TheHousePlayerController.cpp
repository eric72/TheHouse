// Fill out your copyright notice in the Description page of Project Settings.

#include "TheHousePlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Components/InputComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TheHouseObject.h"
#include "UI/TheHouseFPSHudWidget.h"
#include "UI/TheHouseRTSContextMenuWidget.h"
#include "UI/TheHouseRTSMainWidget.h"
#include "UI/TheHouseRTSUITypes.h"
#include "TheHouse/Core/TheHouseHUD.h"
#include "TheHouse/Core/TheHouseSelectable.h"
#include "TheHouseCameraPawn.h"
#include "TheHouseFPSCharacter.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"


namespace {
	/** En Game+UI le focus clavier reste souvent sur un widget Slate : ZQSD
	 * n’atteignent pas le jeu. */
	void TheHouse_FocusKeyboardOnGameViewport() {
	if (FSlateApplication::IsInitialized()) {
		FSlateApplication::Get().SetAllUserFocusToGameViewport(
			EFocusCause::SetDirectly);
	}
	}
} // namespace

namespace TheHousePlacementDebug
{
	static TAutoConsoleVariable<int32> CVarPlacementDebug(
		TEXT("TheHouse.Placement.Debug"),
		0,
		TEXT("0=off, 1=logs for placement flow"),
		ECVF_Default);

	static bool IsEnabled()
	{
		return CVarPlacementDebug.GetValueOnGameThread() != 0;
	}
}

namespace TheHouseFPSDebug
{
	static TAutoConsoleVariable<int32> CVarFPSDebug(
		TEXT("TheHouse.FPS.Debug"),
		0,
		TEXT("0=off, 1=log what the FPS camera is inside/looking at"),
		ECVF_Default);

	static bool IsEnabled()
	{
		return CVarFPSDebug.GetValueOnGameThread() != 0;
	}
}

namespace TheHouseInputPoll {
/** PlayerInput::IsPressed peut rester faux (Enhanced Input / focus Slate) ; la
 * vue jeu reflète l’état matériel. */
bool IsKeyPhysicallyDown(const APlayerController *PC, FKey Key) {
  if (!PC) {
    return false;
  }
  if (PC->IsInputKeyDown(Key)) {
    return true;
  }
  if (UWorld *World = PC->GetWorld()) {
    if (UGameViewportClient *GVC = World->GetGameViewport()) {
      if (GVC->Viewport && GVC->Viewport->KeyState(Key)) {
        return true;
      }
    }
  }
  return false;
}

float PollMoveForward(const APlayerController *PC) {
  float v = 0.f;
  if (IsKeyPhysicallyDown(PC, EKeys::W) || IsKeyPhysicallyDown(PC, EKeys::Z)) {
    v += 1.f;
    FString DebugMsg = FString::Printf(TEXT("Forward V value : %f"), v);
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, DebugMsg);
  }
  if (IsKeyPhysicallyDown(PC, EKeys::S)) {
    v -= 1.f;
    FString DebugMsg = FString::Printf(TEXT("Forward V value : %f"), v);
    // On l'affiche (Note : on utilise %f pour un float, pas %d qui est pour les
    // entiers)
    GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, DebugMsg);
  }
  return FMath::Clamp(v, -1.f, 1.f);
}

float PollMoveRight(const APlayerController *PC) {
  float v = 0.f;
  if (IsKeyPhysicallyDown(PC, EKeys::D)) {
    v += 1.f;
  }
  if (IsKeyPhysicallyDown(PC, EKeys::A) || IsKeyPhysicallyDown(PC, EKeys::Q)) {
    v -= 1.f;
  }
  return FMath::Clamp(v, -1.f, 1.f);
}
} // namespace TheHouseInputPoll

namespace TheHouseInputActions
{
	static const FName RTSZoomModifier(TEXT("TheHouseRTSZoomModifier"));
	static const FName PlacementRotateModifier(TEXT("TheHousePlacementRotateModifier"));
}

bool ATheHousePlayerController::IsRtsZoomMouseModifierHeld() const
{
	if (PlayerInput)
	{
		const TArray<FInputActionKeyMapping>& Maps = PlayerInput->GetKeysForAction(TheHouseInputActions::RTSZoomModifier);
		for (const FInputActionKeyMapping& M : Maps)
		{
			if (M.Key.IsValid() && TheHouseInputPoll::IsKeyPhysicallyDown(this, M.Key))
			{
				return true;
			}
		}
	}

	// Toujours autoriser Shift physique : avec EnhancedPlayerInput, GetKeysForAction peut
	// lister des touches sans que IsKeyPhysicallyDown les voie comme les mêmes FKey.
	return TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::LeftShift) ||
		   TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::RightShift);
}

bool ATheHousePlayerController::IsPlacementRotateModifierHeld() const
{
	if (PlayerInput)
	{
		const TArray<FInputActionKeyMapping>& Maps =
			PlayerInput->GetKeysForAction(TheHouseInputActions::PlacementRotateModifier);
		for (const FInputActionKeyMapping& M : Maps)
		{
			if (M.Key.IsValid() && TheHouseInputPoll::IsKeyPhysicallyDown(this, M.Key))
			{
				return true;
			}
		}
	}

	return TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::LeftControl) ||
		   TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::RightControl);
}

void ATheHousePlayerController::TheHouse_ApplyWheelZoom(float WheelDelta) {
  if (FMath::IsNearlyZero(WheelDelta)) {
    return;
  }

  const bool bShiftMod = IsRtsZoomMouseModifierHeld();
  const bool bCtrlPivotMod = IsPlacementRotateModifierHeld();

  // Molette via GameViewportClient : pivot du preview avec **TheHouseRTSZoomModifier** (Shift)
  // ou **TheHousePlacementRotateModifier** (Ctrl). Shift désactive le zoom caméra.
  if (PlacementState == EPlacementState::Previewing && GetPawn() == RTSPawnReference &&
      PreviewActor && (bShiftMod || bCtrlPivotMod))
  {
    const float Step = FMath::Clamp(PlacementRotationStepDegrees, 1.f, 180.f);
    const float Sign = FMath::Sign(WheelDelta);
    if (!FMath::IsNearlyZero(Sign))
    {
      PlacementPreviewYawDegrees =
          FMath::UnwindDegrees(PlacementPreviewYawDegrees + Sign * Step);
      UpdatePlacementPreview();
    }
    return;
  }

  // Zoom RTS : molette seule (sans Shift). Shift = pas de zoom (réservé au pivot en preview).
  if (bShiftMod) {
    return;
  }

  // Valeur passée à Zoom() : NewLength += Value * RTSZoomSpeed * (-1). Souris
  // haute précision : |WheelDelta| très petit.
  float V = WheelDelta * RTSWheelZoomMultiplier;
  const float ArmDeltaIfApplied = FMath::Abs(V * RTSZoomSpeed);
  if (ArmDeltaIfApplied < RTSMinArmDeltaPerWheelEvent) {
    V = FMath::Sign(V) *
        (RTSMinArmDeltaPerWheelEvent / FMath::Max(RTSZoomSpeed, 1.f));
  }
  Zoom(V);
}

ATheHousePlayerController::ATheHousePlayerController(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  PrimaryActorTick.bCanEverTick = true;

  OverridePlayerInputClass = UPlayerInput::StaticClass();

  bIsRotateModifierDown = false;
  bIsSelecting = false;

  bShowMouseCursor = true;
  bEnableClickEvents = true;
  bEnableMouseOverEvents = true;

  TargetZoomLength = 1000.0f; // Default value matching CameraPawn
}

#pragma region Initialization

void ATheHousePlayerController::BeginPlay() {
	Super::BeginPlay();

	// Listen server / edge init: IsLocallyControlled() chains to
	// IsLocalController(); if bIsLocalPlayerController is not set yet,
	// CharacterMovement will not run ControlledCharacterMove and WASD polling in
	// Tick is skipped.
	if (Cast<ULocalPlayer>(Player)) {
		SetAsLocalPlayerController();
	}

	// Garantit PlayerInput avant le premier Tick (sinon IsInputKeyDown reste faux
	// pour ZQSD / pavé num.).
	if (Cast<ULocalPlayer>(Player) && PlayerInput == nullptr) {
		InitInputSystem();
	}

	if (!IsValid(RTSPawnReference)) {
		RTSPawnReference = Cast<ATheHouseCameraPawn>(GetPawn());
	}

	if (IsValid(RTSPawnReference) && RTSPawnReference->CameraBoom) {
		const float MinL = FMath::Max(200.f, RTSCameraMinArmLength);
		const float MaxL = FMath::Max(MinL + 1.f, RTSCameraMaxArmLength);
		TargetZoomLength = FMath::Clamp(
			RTSPawnReference->CameraBoom->TargetArmLength, MinL, MaxL);
	}

	// RTS : yaw sur le contrôleur ; pitch sur le spring arm (voir
	// ATheHouseCameraPawn::RTSArmPitchDegrees).
	SetControlRotation(FRotator(0.f, 0.f, 0.f));
	if (IsValid(RTSPawnReference)) {
		RTSPawnReference->SetRTSArmPitchDegrees(-60.f);
	}

	// FORCE INPUT MODE ON START
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	TheHouse_FocusKeyboardOnGameViewport();

	SetIgnoreMoveInput(false);

	UE_LOG(LogTemp, Error,
			TEXT("[TheHouse] Révision entrées 2026-04-03 | PlayerInput=%s | Si tu "
				"vois « EnhancedPlayerInput », DefaultInput.ini est encore en "
				"Enhanced — ZQSD/zoom resteront cassés."),
			PlayerInput ? *PlayerInput->GetClass()->GetName() : TEXT("null"));

	UE_LOG(LogTemp, Warning,
			TEXT("[TheHouse] Contrôleur joueur (BeginPlay) | classe=%s | pion=%s "
				"| LocalPlayer=%s | LocalController=%s"),
			*GetClass()->GetName(),
			GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("(aucun)"),
			IsLocalPlayerController() ? TEXT("oui") : TEXT("non"),
			IsLocalController() ? TEXT("oui") : TEXT("non"));

	#if !UE_BUILD_SHIPPING
	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(
			-1, 14.f, FColor::Cyan,
			FString::Printf(
				TEXT("[TheHouse] Entrées = timer (pas seulement Tick) | %s | Pion "
					": %s | Si rien ne change : BP Event Tick doit appeler "
					"Parent, ou Mode de jeu ≠ TheHouse."),
				*GetClass()->GetName(),
				GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("(aucun)")));
	}
	#endif

	TheHouse_StartInputPollTimer();

	EnsureDefaultRTSContextMenuDefs();
	InitializeModeWidgets();
}

void ATheHousePlayerController::ReceivedPlayer() {
  Super::ReceivedPlayer();
  TheHouse_StartInputPollTimer();
}

void ATheHousePlayerController::TheHouse_StartInputPollTimer() {
  if (!Cast<ULocalPlayer>(Player) || !GetWorld()) {
    return;
  }
  if (GetWorldTimerManager().IsTimerActive(TheHouseInputPollTimer)) {
    return;
  }
  GetWorldTimerManager().SetTimer(
      TheHouseInputPollTimer, this,
      &ATheHousePlayerController::TheHouse_PollInputFrame, 1.f / 120.f, true);
  UE_LOG(LogTemp, Warning,
         TEXT("[TheHouse] Timer d'entrées démarré (120 Hz) sur %s | "
              "LocalController=%s"),
         *GetClass()->GetName(),
         IsLocalController() ? TEXT("oui") : TEXT("non"));
}

void ATheHousePlayerController::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
  if (GetWorld()) {
    GetWorldTimerManager().ClearTimer(TheHouseInputPollTimer);
  }

  CloseRTSContextMenu();
  if (RTSMainWidgetInstance)
  {
    RTSMainWidgetInstance->RemoveFromParent();
    RTSMainWidgetInstance = nullptr;
  }
  if (FPSHudWidgetInstance)
  {
    FPSHudWidgetInstance->RemoveFromParent();
    FPSHudWidgetInstance = nullptr;
  }

  Super::EndPlay(EndPlayReason);
}

void ATheHousePlayerController::OnPossess(APawn *InPawn) {
  Super::OnPossess(InPawn);

  SetIgnoreMoveInput(false);

  if (ACharacter *CharacterPawn = Cast<ACharacter>(InPawn)) {
    if (UCharacterMovementComponent *MoveComp =
            CharacterPawn->GetCharacterMovement()) {
      MoveComp->SetActive(true);
    }
  }

  if (ATheHouseCameraPawn *Cam = Cast<ATheHouseCameraPawn>(InPawn)) {
    RTSPawnReference = Cam;
    if (Cam->CameraBoom) {
      const float MinL = FMath::Max(200.f, RTSCameraMinArmLength);
      const float MaxL = FMath::Max(MinL + 1.f, RTSCameraMaxArmLength);
      TargetZoomLength =
          FMath::Clamp(Cam->CameraBoom->TargetArmLength, MinL, MaxL);
    }
  }

  TheHouse_StartInputPollTimer();

  UpdateModeWidgetsVisibility();
}

void ATheHousePlayerController::Tick(float DeltaTime) {
  Super::Tick(DeltaTime);
  // Secours si BeginPlay/ReceivedPlayer n’ont pas lancé le timer (BP sans
  // Super, ordre d’init).
  if (GetWorld() && Cast<ULocalPlayer>(Player) &&
      !GetWorldTimerManager().IsTimerActive(TheHouseInputPollTimer)) {
    TheHouse_StartInputPollTimer();
  }

  // Déplacement + zoom (ignorés si move input coupé — le pitch RTS reste géré
  // après).
  if (Cast<ULocalPlayer>(Player) && GetPawn() && !IsMoveInputIgnored()) {
    float Fwd = 0.f;
    float Rt = 0.f;
    if (PlayerInput) {
      Fwd = GetInputAxisValue(TEXT("MoveForward"));
      Rt = GetInputAxisValue(TEXT("MoveRight"));
    }

    if (FMath::Abs(Fwd) < 0.02f) {
      Fwd = TheHouseInputPoll::PollMoveForward(this);
    }

    if (FMath::Abs(Rt) < 0.02f) {
      Rt = TheHouseInputPoll::PollMoveRight(this);
    }

    if (!FMath::IsNearlyZero(Fwd)) {
      MoveForward(Fwd);
    }
    if (!FMath::IsNearlyZero(Rt)) {
      MoveRight(Rt);
    }

    if (RTSZoomInterpSpeed > KINDA_SMALL_NUMBER) {
      if (ATheHouseCameraPawn *Cam = Cast<ATheHouseCameraPawn>(GetPawn())) {
        if (Cam->CameraBoom &&
            !FMath::IsNearlyEqual(Cam->CameraBoom->TargetArmLength,
                                  TargetZoomLength, 0.25f)) {
          Cam->CameraBoom->TargetArmLength =
              FMath::FInterpTo(Cam->CameraBoom->TargetArmLength,
                               TargetZoomLength, DeltaTime, RTSZoomInterpSpeed);
        }
      }
    }
  }

  // Pitch RTS (Alt / clic droit) : l’axe LookUp peut rester à 0 ; avec souris
  // capturée, GetMousePosition ne bouge presque pas (curseur recentré) → pas de
  // pitch. GetInputMouseDelta garde le delta brut.
  if (Cast<ULocalPlayer>(Player) && bIsRotateModifierDown) {
    if (ATheHouseCameraPawn *Rts = Cast<ATheHouseCameraPawn>(GetPawn())) {
      float AxisLook = 0.f;
      if (PlayerInput) {
        AxisLook = GetInputAxisValue(TEXT("LookUp"));
      }
      // -1 : même mapping que le FPS (souris vers le haut = relever la vue).
      // Sans ça, le boom + capture souris donnent l’axe Y inversé.
      constexpr float RTSPitchVerticalSign = -1.f;
      if (FMath::Abs(AxisLook) > 0.0001f) {
        Rts->ApplyRTSArmPitchDelta(RTSPitchVerticalSign * AxisLook *
                                   RotationSensitivity);
      } else {
        float RawDX = 0.f, RawDY = 0.f;
        GetInputMouseDelta(RawDX, RawDY);
        if (!FMath::IsNearlyZero(RawDY)) {
          Rts->ApplyRTSArmPitchDelta(RTSPitchVerticalSign * RawDY * 0.25f *
                                     RotationSensitivity);
        }
      }
    }
  }

  // if (IsInputKeyDown(EKeys::P))
  // {
  //   UE_LOG(LogTemp, Warning, TEXT("P DETECTED VIA TICK"));
  // }
}

void ATheHousePlayerController::TheHouse_PollInputFrame() {
  // Même critère que le démarrage du timer (IsLocalPlayerController exige
  // IsLocalController ; peut être faux trop tôt).
  if (!Cast<ULocalPlayer>(Player) || !GetPawn()) {
    return;
  }

  // Déplacement + souris : BindAxis + DefaultInput.ini +
  // DefaultPlayerInputClass=PlayerInput (obligatoire). Le polling ZQSD ici
  // doublait ou restait à 0 selon focus / layout ; GetInputMouseDelta ignorait
  // la sensibilité moteur (0.07) → sens « bugué ».

  // Zoom RTS de secours — Pavé num. + / - (comme la molette : sans Shift ; Shift bloque le zoom).
  if (Cast<ATheHouseCameraPawn>(GetPawn()) && !IsRtsZoomMouseModifierHeld()) {
    // ~8 uu de variation de bras par frame (ralenti vs ancien 15 ; indépendant
    // de RTSZoomSpeed via la formule).
    const float ZoomKeyStep = 8.f / FMath::Max(RTSZoomSpeed, 1.f);
    if (TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::Add)) {
      Zoom(ZoomKeyStep);
    }
    if (TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::Subtract)) {
      Zoom(-ZoomKeyStep);
    }
  }

  if (bIsSelecting) {
    float MouseX, MouseY;
    if (GetMousePosition(MouseX, MouseY)) {
      if (ATheHouseHUD *HUD = Cast<ATheHouseHUD>(GetHUD())) {
        HUD->UpdateSelection(FVector2D(MouseX, MouseY));
      }
    }
  }

  // Menu contextuel RTS : Game+UI mange souvent InputKey avant le jeu ; front montant
  // via l’état physique (comme mouvement) + BindAction("RTSObjectContext").
  {
    const bool bRMB = TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::RightMouseButton);
    if (!bPrevPollRMBHeld && bRMB && Cast<ATheHouseCameraPawn>(GetPawn()) == RTSPawnReference &&
        RTSPawnReference && !IsRtsCameraRotateModifierPhysicallyDown())
    {
      TryOpenRTSContextMenuAtCursor();
    }
    bPrevPollRMBHeld = bRMB;
  }
}

void ATheHousePlayerController::SetupInputComponent() {
  // Force classic UInputComponent: project INI / editor often reset to
  // EnhancedInputComponent, which breaks legacy BindAction here.
  if (InputComponent == nullptr) {
    InputComponent = NewObject<UInputComponent>(
        this, UInputComponent::StaticClass(), TEXT("PC_InputComponent0"));
    InputComponent->RegisterComponent();
  }

  APlayerController::SetupInputComponent();

  // MoveForward/MoveRight : on force leur écoute native continue !
  // (Indispensable pour GameOnly en mode FPS)
  InputComponent->BindAxis(TEXT("MoveForward"), this,
                           &ATheHousePlayerController::MoveForward);
  InputComponent->BindAxis(TEXT("MoveRight"), this,
                           &ATheHousePlayerController::MoveRight);

  InputComponent->BindAxis(TEXT("Turn"), this,
                           &ATheHousePlayerController::RotateCamera);
  InputComponent->BindAxis(TEXT("LookUp"), this,
                           &ATheHousePlayerController::LookUp);

  // Actions
  InputComponent->BindAction("DebugToggleFPS", IE_Pressed, this,
                             &ATheHousePlayerController::DebugSwitchToFPS);
  InputComponent->BindAction(
      "RotateModifier", IE_Pressed, this,
      &ATheHousePlayerController::OnRotateModifierPressed);
  InputComponent->BindAction(
      "RotateModifier", IE_Released, this,
      &ATheHousePlayerController::OnRotateModifierReleased);

  InputComponent->BindAction(
    "DebugPlaceObject",
    IE_Pressed,
    this,
    &ATheHousePlayerController::DebugStartPlacement
  );

  // Selection / confirmation (Left click). If we are in placement preview,
  // clicking confirms instead of selecting.
  InputComponent->BindAction("Select", IE_Pressed, this,
                             &ATheHousePlayerController::Input_SelectPressed);
  InputComponent->BindAction("Select", IE_Released, this,
                             &ATheHousePlayerController::Input_SelectReleased);

  InputComponent->BindAction(
      "CancelOrCloseRts",
      IE_Pressed,
      this,
      &ATheHousePlayerController::Input_CancelOrCloseRts);

  InputComponent->BindAction(
      "RTSObjectContext",
      IE_Pressed,
      this,
      &ATheHousePlayerController::Input_RTSObjectContext);
}

void ATheHousePlayerController::DebugStartPlacement()
{
	// Placement is RTS-only.
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}

	// Toggle: pressing P again cancels current preview.
	if (PlacementState == EPlacementState::Previewing)
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] P pressed: cancel preview"));
		}
		CancelPlacement();
		return;
	}

	if (PlacementState != EPlacementState::None)
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] P pressed but state != None (%d)"), (int32)PlacementState);
		}
		return;
	}

	if (!PendingPlacementClass)
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Error, TEXT("[Placement] PendingPlacementClass is NULL"));
		}
		return;
	}

	SpawnPlacementPreviewFromPendingClass();
}

void ATheHousePlayerController::CancelPlacement()
{
	if (PreviewActor)
	{
		PreviewActor->Destroy();
		PreviewActor = nullptr;
	}
	PlacementState = EPlacementState::None;
	PlacementPreviewYawDegrees = 0.f;
	PendingStockConsumeIndex = INDEX_NONE;
}

void ATheHousePlayerController::SpawnPlacementPreviewFromPendingClass()
{
	if (!GetWorld() || !PendingPlacementClass)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	PreviewActor = GetWorld()->SpawnActor<ATheHouseObject>(
		PendingPlacementClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		Params
	);

	if (!PreviewActor)
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Error, TEXT("[Placement] Spawn preview FAILED (class=%s)"),
				*PendingPlacementClass->GetPathName());
		}
		return;
	}

	if (TheHousePlacementDebug::IsEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Placement] Spawned preview=%s class=%s"),
			*PreviewActor->GetName(),
			*PendingPlacementClass->GetPathName());
	}

	PlacementPreviewYawDegrees = 0.f;
	PreviewActor->SetAsPreview(true);

	PlacementState = EPlacementState::Previewing;
}

void ATheHousePlayerController::StartPlacementPreviewForClass(TSubclassOf<ATheHouseObject> InClass)
{
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}
	if (!InClass)
	{
		return;
	}

	PendingStockConsumeIndex = INDEX_NONE;
	PendingPlacementClass = InClass;
	CancelPlacement();
	SpawnPlacementPreviewFromPendingClass();
}

void ATheHousePlayerController::ConsumeStoredPlaceableAndBeginPreview(int32 StoredIndex)
{
	if (!StoredPlaceables.IsValidIndex(StoredIndex))
	{
		return;
	}

	PendingPlacementClass = StoredPlaceables[StoredIndex];
	CancelPlacement();
	PendingStockConsumeIndex = StoredIndex;
	SpawnPlacementPreviewFromPendingClass();
}

#pragma endregion

#pragma region Input Handlers

void ATheHousePlayerController::OnRotateModifierPressed() {
  bIsRotateModifierDown = true;

  // If the user starts rotating the camera, cancel any ongoing selection box.
  if (bIsSelecting) {
    bIsSelecting = false;
    if (ATheHouseHUD *HUD = Cast<ATheHouseHUD>(GetHUD())) {
      HUD->StopSelection();
    }
  }

  // In RTS Mode, HIDE cursor and LOCK it to allow infinite scrolling/rotation
  // without hitting edge
  if (Cast<ATheHouseCameraPawn>(GetPawn())) {
    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    TheHouse_FocusKeyboardOnGameViewport();
  }
}

void ATheHousePlayerController::OnRotateModifierReleased() {
  bIsRotateModifierDown = false;

  // In RTS Mode, SHOW cursor and UNLOCK it to allow clicking UI/Selection
  if (Cast<ATheHouseCameraPawn>(GetPawn())) {
    bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
    TheHouse_FocusKeyboardOnGameViewport();
  }
}

void ATheHousePlayerController::MoveForward(float Value) {
  if (Value == 0.0f)
    return;

  APawn *ControlledPawn = GetPawn();
  if (ControlledPawn) {
    if (ACharacter *Char = Cast<ACharacter>(ControlledPawn)) {
      // Même logique que la caméra RTS : direction = yaw du contrôleur (FPS
      // fiable même si capsule / mesh décalés).
      const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
      const FVector FwdDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
      Char->AddMovementInput(FwdDir, Value, true);
    } else if (ATheHouseCameraPawn *Rts =
                   Cast<ATheHouseCameraPawn>(ControlledPawn)) {
      // Camera-Relative Forward/Back
      // We use the Controller's Yaw because the CameraBoom inherits it.
      FRotator YawRotation(0, GetControlRotation().Yaw, 0);
      FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

      // Standard "W = Forward" logic.
      // If previous inversion was due to bad orientation, this fixes it
      // naturally.
      Rts->AddMovementInput(Direction, Value, true);
    }
  }
}

void ATheHousePlayerController::MoveRight(float Value) {
  if (Value == 0.0f)
    return;

  APawn *ControlledPawn = GetPawn();
  if (ControlledPawn) {
    if (ACharacter *Char = Cast<ACharacter>(ControlledPawn)) {
      Char->AddMovementInput(Char->GetActorRightVector(), Value, true);
    } else if (ATheHouseCameraPawn *Rts =
                   Cast<ATheHouseCameraPawn>(ControlledPawn)) {
      // Camera-Relative Right/Left
      FRotator YawRotation(0, GetControlRotation().Yaw, 0);
      FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

      // Standard RTS Movement (No Inversion)
      Rts->AddMovementInput(Direction, Value, true);
    }
  }
}

void ATheHousePlayerController::Zoom(float Value) {
  if (Value == 0.0f)
    return;

  ATheHouseCameraPawn *Cam = Cast<ATheHouseCameraPawn>(GetPawn());
  if (!IsValid(Cam) && IsValid(RTSPawnReference) &&
      GetPawn() == RTSPawnReference) {
    Cam = RTSPawnReference;
  }
  if (!IsValid(Cam) || !Cam->CameraBoom) {
    return;
  }

  const float OldLength = Cam->CameraBoom->TargetArmLength;
  float ScaledValue = Value;
  if (bRTSZoomScalesWithArmLength && RTSZoomArmLengthRef > KINDA_SMALL_NUMBER) {
    // Vue très éloignée : un cran de molette change nettement la distance
    // perçue.
    ScaledValue *= FMath::Clamp(OldLength / RTSZoomArmLengthRef, 0.5f, 6.f);
  }
  const float MinL = FMath::Max(200.f, RTSCameraMinArmLength);
  const float MaxL = FMath::Max(MinL + 1.f, RTSCameraMaxArmLength);
  const float NewLength =
      FMath::Clamp(OldLength + (ScaledValue * RTSZoomSpeed * -1.f), MinL, MaxL);
  TargetZoomLength = NewLength;
  if (RTSZoomInterpSpeed <= KINDA_SMALL_NUMBER) {
    Cam->CameraBoom->TargetArmLength = NewLength;
  }

#if !UE_BUILD_SHIPPING
  if (bDebugLogRTSZoom && Cam->CameraBoom) {
    const float Effective =
        FVector::Distance(Cam->GetActorLocation(),
                          Cam->RTSCameraComponent
                              ? Cam->RTSCameraComponent->GetComponentLocation()
                              : Cam->GetActorLocation());
    UE_LOG(LogTemp, Log,
           TEXT("[TheHouse] Zoom | TargetArm=%.0f→%.0f | interp=%.1f | "
                "boomCollision=%s | dist cam≈%.0f"),
           OldLength, NewLength, RTSZoomInterpSpeed,
           Cam->CameraBoom->bDoCollisionTest ? TEXT("oui") : TEXT("non"),
           Effective);
  }
#endif

  // Zoom « vers le curseur » le long de l’axe de visée. L’ancien pan avec
  // ToHit.Z=0 forçait l’horizontal monde : combiné au boom incliné, le ressenti
  // était surtout un zoom « vertical » au lieu de suivre l’angle caméra.
  FHitResult Hit;
  if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) &&
      OldLength > KINDA_SMALL_NUMBER && Cam->RTSCameraComponent) {
    const FVector CamLoc = Cam->RTSCameraComponent->GetComponentLocation();
    const FVector ViewFwd =
        Cam->RTSCameraComponent->GetForwardVector().GetSafeNormal();
    const float ZoomDelta = OldLength - NewLength;
    const float Fraction = ZoomDelta / OldLength;
    const float DepthAlongView =
        FVector::DotProduct(Hit.ImpactPoint - CamLoc, ViewFwd);
    if (DepthAlongView > KINDA_SMALL_NUMBER) {
      const float Depth = FMath::Max(DepthAlongView, 50.f);
      FVector Pan = ViewFwd * (Depth * Fraction);
      if (RTSCursorZoomMaxPanPerEvent > 0.f) {
        const float M = RTSCursorZoomMaxPanPerEvent;
        if (Pan.SizeSquared() > M * M) {
          Pan = Pan.GetSafeNormal() * M;
        }
      }
      Cam->AddActorWorldOffset(Pan);
    }
  }

  ClampRtsPawnAboveGround(Cam);
}

void ATheHousePlayerController::ClampRtsPawnAboveGround(ATheHouseCameraPawn* Cam)
{
	if (!Cam || !GetWorld())
	{
		return;
	}

	FVector Loc = Cam->GetActorLocation();
	const FVector TraceStart = Loc + FVector(0.f, 0.f, 6000.f);
	const FVector TraceEnd = Loc - FVector(0.f, 0.f, 200000.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TheHouseRTSPawnGroundClamp), false);
	Params.AddIgnoredActor(Cam);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) || !Hit.bBlockingHit)
	{
		return;
	}

	const float MinZ = Hit.ImpactPoint.Z + RTSPawnMinClearanceAboveGroundUU;
	if (Loc.Z < MinZ)
	{
		Loc.Z = MinZ;
		Cam->SetActorLocation(Loc);
	}
}

void ATheHousePlayerController::RotateCamera(float Value) {
  if (FMath::IsNearlyZero(Value)) {
    return;
  }
  // Value intègre déjà MouseX × sensibilité projet (DefaultInput / AxisConfig
  // ~0.07).
  if (Cast<ATheHouseCameraPawn>(GetPawn())) {
    if (!bIsRotateModifierDown) {
      return;
    }
    AddYawInput(Value * RotationSensitivity);
  } else if (Cast<ACharacter>(GetPawn()) && !bShowMouseCursor) {
    AddYawInput(Value * RotationSensitivity * 0.5f);
  }
}

void ATheHousePlayerController::LookUp(float Value) {
  if (FMath::IsNearlyZero(Value)) {
    return;
  }
  // RTS : le pitch est appliqué dans Tick (axe LookUp + repli
  // GetInputMouseDelta si la souris est capturée).
  if (Cast<ATheHouseCameraPawn>(GetPawn())) {
    return;
  }
  if (Cast<ACharacter>(GetPawn()) && !bShowMouseCursor) {
    AddPitchInput(Value * RotationSensitivity * 0.5f);
  }
}

#pragma endregion

#pragma region Mode Switching

void ATheHousePlayerController::SwitchToRTS() {
  if (!IsValid(RTSPawnReference)) {
    UE_LOG(LogTemp, Warning, TEXT("RTS Pawn Reference is missing!"));
    return;
  }

  // Placement preview is RTS-only.
  CancelPlacement();

  // 1. Sync RTS Camera to FPS position — seulement si la position FPS est
  // raisonnable (< 2000 uu de haut) pour éviter la cascade de hauteur.
  if (IsValid(ActiveFPSCharacter)) {
    FVector FPSLoc = ActiveFPSCharacter->GetActorLocation();
    if (FPSLoc.Z < 2000.f) {
      RTSPawnReference->SetActorLocation(FPSLoc + FVector(0.f, 0.f, 150.f));
    }
    // Si le FPS était dans le ciel (bug de spawn), on NE téléporte PAS le pivot.
  }

  {
    FRotator Ctrl = GetControlRotation();
    Ctrl.Pitch = 0.f;
    Ctrl.Roll = 0.f;
    SetControlRotation(Ctrl);
    RTSPawnReference->SetRTSArmPitchDegrees(-60.f);
  }

  // 2. Possess
  Possess(RTSPawnReference);
  SetIgnoreMoveInput(false);

  // 3. Cleanup FPS Character
  if (IsValid(ActiveFPSCharacter)) {
    ActiveFPSCharacter->Destroy();
    ActiveFPSCharacter = nullptr;
  }

  // 4. Input Mode (Show Cursor)
  bShowMouseCursor = true;
  FInputModeGameAndUI InputMode;
  InputMode.SetLockMouseToViewportBehavior(
      EMouseLockMode::LockAlways); // Keep mouse in window!
  InputMode.SetHideCursorDuringCapture(false);
  SetInputMode(InputMode);
  TheHouse_FocusKeyboardOnGameViewport();

  CloseRTSContextMenu();
  UpdateModeWidgetsVisibility();
}

void ATheHousePlayerController::SwitchToFPS(FVector TargetLocation, FRotator TargetRotation) {
	if (!FPSCharacterClass) {
		FPSCharacterClass = ATheHouseFPSCharacter::StaticClass();
	}

	// Placement preview is RTS-only.
	CancelPlacement();
	CloseRTSContextMenu();

	// TargetLocation est déjà le sol réel fourni par DebugSwitchToFPS
	// (via GetHitResultUnderCursor). On ajoute juste la demi-hauteur capsule.
	constexpr float CapsuleHalfHeight = 196.f;
	const FVector FPSSpawnLoc = TargetLocation + FVector(0.f, 0.f, CapsuleHalfHeight);

	#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("[TheHouse] SwitchToFPS | ImpactZ=%.0f -> SpawnZ=%.0f"), TargetLocation.Z, FPSSpawnLoc.Z);
	#endif

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	ActiveFPSCharacter = GetWorld()->SpawnActor<ATheHouseFPSCharacter>(
		FPSCharacterClass, FPSSpawnLoc, TargetRotation, SpawnParams);
	// ActiveFPSCharacter = GetWorld()->SpawnActor<ATheHouseFPSCharacter>(
	// 	FPSCharacterClass, FVector(0.f, 0.f, 0.f), TargetRotation, SpawnParams);

	if (ActiveFPSCharacter) {
		Possess(ActiveFPSCharacter);
		SetIgnoreMoveInput(false);

		// Hide debug/test cube components on the FPS pawn (prevents a giant cube in front of camera).
		// If you actually want a visible mesh on the FPS pawn, rename it or add tag "KeepVisibleInFPS".
		{
			TArray<UStaticMeshComponent*> MeshComps;
			ActiveFPSCharacter->GetComponents<UStaticMeshComponent>(MeshComps);
			for (UStaticMeshComponent* C : MeshComps)
			{
				if (!C)
				{
					continue;
				}
				if (C->ComponentHasTag(TEXT("KeepVisibleInFPS")))
				{
					continue;
				}

				const bool bLooksLikeTestCube =
					C->GetName().Equals(TEXT("Cube"), ESearchCase::IgnoreCase) ||
					(C->GetStaticMesh() && C->GetStaticMesh()->GetName().Equals(TEXT("Cube"), ESearchCase::IgnoreCase));

				if (bLooksLikeTestCube)
				{
					C->SetVisibility(false, true);
					C->SetHiddenInGame(true, true);
					C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
					C->SetGenerateOverlapEvents(false);
				}
			}
		}

		// Gravité : ACharacter::Restart() (appelé par Possess) réinitialise le
		// MovementMode. On la force 0.1s plus tard, après que Restart() soit terminé,
		// pour s'assurer que le personnage tombe s'il est dans le vide.
		ATheHouseFPSCharacter* FPSRef = ActiveFPSCharacter;
		FTimerHandle GravityTimer;
		GetWorldTimerManager().SetTimer(GravityTimer, FTimerDelegate::CreateLambda([FPSRef]() {
		if (IsValid(FPSRef)) {
			if (UCharacterMovementComponent* CMC = FPSRef->GetCharacterMovement()) {
			if (!CMC->IsMovingOnGround()) {
				CMC->GravityScale = 1.0f;
				CMC->SetMovementMode(MOVE_Falling);
			}
			}
		}
		}), 0.1f, false);

		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		TheHouse_FocusKeyboardOnGameViewport();
	} else {
		UE_LOG(LogTemp, Error, TEXT("[TheHouse] SwitchToFPS | SpawnActor nullptr !"));
	}

	UpdateModeWidgetsVisibility();
}


void ATheHousePlayerController::DebugSwitchToFPS() {
  APawn *CurrentPawn = GetPawn();

  if (Cast<ACharacter>(CurrentPawn) &&
      !Cast<ATheHouseCameraPawn>(CurrentPawn)) {
    SwitchToRTS();
    return;
  }

  if (!IsValid(RTSPawnReference)) return;

  // -------------------------------------------------------------------
  // SPAWN GROUND POINT
  // On trace depuis la caméra à travers la position de la souris (deproject)
  // dans la direction de visée de la caméra. C'est exactement ce que le
  // joueur voit — fiable sur terrain ouvert et en intérieur.
  // On n'utilise PAS de trace verticale depuis une hauteur fixe car cela
  // frappe la skybox ou d'autres géométries hautes.
  // -------------------------------------------------------------------
  FVector SpawnGroundPoint = FVector::ZeroVector;
  bool bGotFloor = false;

  {
    FVector WorldOrigin, WorldDir;
    if (DeprojectMousePositionToWorld(WorldOrigin, WorldDir)) {
      FHitResult Hit;
      FCollisionQueryParams Params;
      Params.bTraceComplex = true;
      Params.AddIgnoredActor(RTSPawnReference); // ignore le pivot RTS
      const FVector TraceEnd = WorldOrigin + WorldDir * 100000.f;
      if (GetWorld()->LineTraceSingleByChannel(
              Hit, WorldOrigin, TraceEnd, ECC_WorldStatic, Params) &&
          Hit.ImpactNormal.Z > 0.1f) {
        SpawnGroundPoint = Hit.ImpactPoint;
        bGotFloor = true;
      }
      if (!bGotFloor) {
        if (GetWorld()->LineTraceSingleByChannel(
                Hit, WorldOrigin, TraceEnd, ECC_Visibility, Params) &&
            Hit.ImpactNormal.Z > 0.1f) {
          SpawnGroundPoint = Hit.ImpactPoint;
          bGotFloor = true;
        }
      }
    }
  }

  // Dernier recours : pivot RTS. La gravité (timer 0.1s dans SwitchToFPS)
  // fera tomber le personnage s'il est dans le vide.
  if (!bGotFloor) {
    SpawnGroundPoint = RTSPawnReference->GetActorLocation();
    UE_LOG(LogTemp, Warning,
           TEXT("[TheHouse] DebugSwitchToFPS | Deproject/trace échoué, "
                "fallback pivot (Z=%.0f). Le personnage tombera si en l'air."),
           SpawnGroundPoint.Z);
  }

	#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning,
        TEXT("[TheHouse] DebugSwitchToFPS | GroundZ=%.0f | OK=%s"),
        SpawnGroundPoint.Z,
        bGotFloor ? TEXT("oui") : TEXT("fallback"));
    #endif

  const FRotator SpawnRot(0.f, GetControlRotation().Yaw, 0.f);
  SwitchToFPS(SpawnGroundPoint, SpawnRot);
}


void ATheHousePlayerController::Input_SelectPressed() {
  // While rotating (ALT / RMB), left click should not start selection nor confirm placement.
  if (bIsRotateModifierDown) {
    return;
  }

  if (PlacementState == EPlacementState::Previewing) {
    ConfirmPlacement();
    return;
  }
  if (!RTSPawnReference || GetPawn() != RTSPawnReference)
    return;

  // Start Selection
  float MouseX, MouseY;
  if (GetMousePosition(MouseX, MouseY)) {
    bIsSelecting = true;
    SelectionStartPos = FVector2D(MouseX, MouseY);

    // Notify HUD
    if (ATheHouseHUD *HUD = Cast<ATheHouseHUD>(GetHUD())) {
      HUD->StartSelection(SelectionStartPos);
    }
  }
}

void ATheHousePlayerController::Input_SelectReleased() {
  // While rotating (ALT / RMB), ignore selection release too.
  if (bIsRotateModifierDown) {
    return;
  }

  bIsSelecting = false;

  // Notify HUD to stop drawing
  if (ATheHouseHUD *HUD = Cast<ATheHouseHUD>(GetHUD())) {
    HUD->StopSelection();
  }

  if (!IsValid(RTSPawnReference) || GetPawn() != RTSPawnReference)
    return;

  float MouseX, MouseY;
  if (!GetMousePosition(MouseX, MouseY))
    return;

  FVector2D CurrentMousePos(MouseX, MouseY);
  float DragDistance = FVector2D::Distance(SelectionStartPos, CurrentMousePos);

  // Log for debug
  // UE_LOG(LogTemp, Log, TEXT("Selection Released. Drag Dist: %f"),
  // DragDistance);

  if (DragDistance < 10.0f) // Small movement = Single Click
  {
    // Single Click Selection
    // KEY POINT: We use ECC_Visibility.
    // Our SmartWall disables its Visibility collision when faded, so this trace
    // will naturally ignore it!
    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit)) {
      AActor *HitActor = Hit.GetActor();

      // Check Interface AND IsSelectable condition
      // We access the interface via the ITheHouseSelectable pointer to call the
      // virtual function correctly
      if (HitActor && HitActor->Implements<UTheHouseSelectable>()) {
        ITheHouseSelectable *Selectable = Cast<ITheHouseSelectable>(HitActor);
        if (Selectable && Selectable->IsSelectable()) {
          // Deselect all others first (Simple logic for now)
          // TODO: Implement Multi-Select with Shift
          // For now, just Log hitting a selectable
          UE_LOG(LogTemp, Warning, TEXT("Selected Actor: %s"),
                 *HitActor->GetName());

          Selectable->OnSelect();
        }
      }
    }
  } else // Box Selection
  {
    TArray<AActor *> SelectedActors;

    if (AHUD *HUD = GetHUD()) {
      HUD->GetActorsInSelectionRectangle<AActor>(
          SelectionStartPos, CurrentMousePos, SelectedActors, false, false);
    }

    for (AActor *Actor : SelectedActors) {
      // Check Interface AND IsSelectable condition
      if (Actor && Actor->Implements<UTheHouseSelectable>()) {
        ITheHouseSelectable *Selectable = Cast<ITheHouseSelectable>(Actor);
        if (Selectable && Selectable->IsSelectable()) {
          UE_LOG(LogTemp, Warning, TEXT("Box Selected: %s"), *Actor->GetName());
          Selectable->OnSelect();
        }
      }
    }
  }
}


// Object placemet on the map

void ATheHousePlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

    const bool bIsInRTSPlacementContext = (GetPawn() == RTSPawnReference);

    // Placement rotation : géré dans TheHouse_ApplyWheelZoom (molette interceptée par le viewport).

    if (bIsInRTSPlacementContext && PlacementState == EPlacementState::Previewing && PreviewActor)
    {
      UpdatePlacementPreview();
    }

    // Fallback confirm: independent of ActionMappings/EnhancedInput quirks.
    // If we're previewing and the user left-clicks (without rotate modifier), confirm placement.
    if (bIsInRTSPlacementContext && PlacementState == EPlacementState::Previewing && PreviewActor && !bIsRotateModifierDown)
    {
      if (WasInputKeyJustPressed(EKeys::LeftMouseButton))
      {
        ConfirmPlacement();
      }
    }

    // FPS debug: identify the big cube / blocking actor in front of camera.
    if (TheHouseFPSDebug::IsEnabled() && GetPawn() && GetPawn() != RTSPawnReference)
    {
      static double LastLogTimeSeconds = 0.0;
      const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
      if (Now - LastLogTimeSeconds > 0.75)
      {
        LastLogTimeSeconds = Now;

        FVector CamLoc;
        FRotator CamRot;
        GetPlayerViewPoint(CamLoc, CamRot);

        FHitResult Hit;
        const FVector End = CamLoc + CamRot.Vector() * 250.f;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(FPSDebugTrace), true);
        Params.AddIgnoredActor(GetPawn());

        if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, CamLoc, End, ECC_Visibility, Params) && Hit.bBlockingHit)
        {
          UE_LOG(LogTemp, Warning, TEXT("[FPSDebug] Hit actor=%s comp=%s dist=%.1f loc=%s"),
            Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"),
            Hit.GetComponent() ? *Hit.GetComponent()->GetName() : TEXT("null"),
            Hit.Distance,
            *Hit.ImpactPoint.ToString());
        }
        else
        {
          UE_LOG(LogTemp, Warning, TEXT("[FPSDebug] No hit ahead. Pawn=%s CamLoc=%s"), *GetPawn()->GetName(), *CamLoc.ToString());
        }

        // If we're inside geometry, a forward line trace can miss depending on collision settings.
        // Do a tiny sweep at the camera to list blocking components around us.
        if (GetWorld())
        {
          TArray<FHitResult> SweepHits;
          const FCollisionShape Shape = FCollisionShape::MakeSphere(12.f);
          const bool bAnySweep = GetWorld()->SweepMultiByChannel(
            SweepHits,
            CamLoc,
            CamLoc,
            FQuat::Identity,
            ECC_Visibility,
            Shape,
            Params
          );

          if (bAnySweep && SweepHits.Num() > 0)
          {
            int32 Printed = 0;
            for (const FHitResult& H : SweepHits)
            {
              if (!H.bBlockingHit && !H.bStartPenetrating)
              {
                continue;
              }
              UE_LOG(LogTemp, Warning, TEXT("[FPSDebug] Sweep hit actor=%s comp=%s startPen=%s penDepth=%.2f"),
                H.GetActor() ? *H.GetActor()->GetName() : TEXT("null"),
                H.GetComponent() ? *H.GetComponent()->GetName() : TEXT("null"),
                H.bStartPenetrating ? TEXT("true") : TEXT("false"),
                H.PenetrationDepth);

              if (++Printed >= 4) break;
            }
          }
        }

        // Log any visible StaticMeshComponents on the FPS pawn (this often reveals a "debug cube" component).
        {
          TArray<UStaticMeshComponent*> MeshComps;
          GetPawn()->GetComponents<UStaticMeshComponent>(MeshComps);
          for (UStaticMeshComponent* C : MeshComps)
          {
            if (!C) continue;
            if (C->IsVisible() && !C->bHiddenInGame)
            {
              const FString MeshName = C->GetStaticMesh() ? C->GetStaticMesh()->GetName() : TEXT("null");
              UE_LOG(LogTemp, Warning, TEXT("[FPSDebug] Pawn mesh comp=%s mesh=%s hiddenInGame=%s loc=%s"),
                *C->GetName(),
                *MeshName,
                C->bHiddenInGame ? TEXT("true") : TEXT("false"),
                *C->GetComponentLocation().ToString());
            }
          }
        }

        // Also list any visible primitives (helps if the cube is a non-static primitive).
        {
          TArray<UPrimitiveComponent*> PrimComps;
          GetPawn()->GetComponents<UPrimitiveComponent>(PrimComps);
          int32 Printed = 0;
          for (UPrimitiveComponent* P : PrimComps)
          {
            if (!P) continue;
            if (P->IsVisible() && !P->bHiddenInGame)
            {
              UE_LOG(LogTemp, Warning, TEXT("[FPSDebug] Pawn prim comp=%s class=%s"),
                *P->GetName(),
                *P->GetClass()->GetName());
              if (++Printed >= 6) break;
            }
          }
        }

        if (AActor* PawnActor = GetPawn())
        {
          TArray<AActor*> Attached;
          PawnActor->GetAttachedActors(Attached);
          if (Attached.Num() > 0)
          {
            FString Names;
            for (AActor* A : Attached)
            {
              Names += A ? (A->GetName() + TEXT(" ")) : TEXT("null ");
            }
            UE_LOG(LogTemp, Warning, TEXT("[FPSDebug] AttachedActors: %s"), *Names);
          }
        }
      }
    }
}

void ATheHousePlayerController::UpdatePlacementPreview()
{
	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] DeprojectMousePositionToWorld FAILED"));
		}
		return;
	}

	FHitResult Hit;
	const FVector End = WorldOrigin + WorldDir * 100000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlacementPreviewTrace), /*bTraceComplex*/ true);
	Params.AddIgnoredActor(this);
	if (RTSPawnReference)
	{
		Params.AddIgnoredActor(RTSPawnReference);
	}
	if (PreviewActor)
	{
		Params.AddIgnoredActor(PreviewActor);
	}

	if (GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, End, ECC_Visibility, Params))
	{
		if (!Hit.bBlockingHit)
		{
			PreviewActor->SetPlacementState(EObjectPlacementState::OverlapsWorld);
			if (TheHousePlacementDebug::IsEnabled())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Placement] Trace hit but not blocking"));
			}
			return;
		}

		// Reject walls/steep surfaces: placement grid is meant for floors.
		if (!ValidatePlacement(Hit))
		{
			PreviewActor->SetPlacementState(EObjectPlacementState::OverlapsWorld);
			if (TheHousePlacementDebug::IsEnabled())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Placement] Invalid surface normal Z=%.3f actor=%s"),
					Hit.ImpactNormal.Z,
					Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("null"));
			}
			return;
		}

		FVector Loc = Hit.ImpactPoint;
		if (PlacementGrid.bEnableGridSnap && PlacementGrid.CellSize > KINDA_SMALL_NUMBER)
		{
			Loc = SnapWorldToGridXY(Loc);
		}

		// Keep Z from hit (allows terrain height), but snap XY to grid.
		FTransform CandidateTransform = PreviewActor->GetActorTransform();
		CandidateTransform.SetLocation(Loc);
		CandidateTransform.SetRotation(FQuat(FRotator(0.f, PlacementPreviewYawDegrees, 0.f)));

		// Ignore the surface under cursor for "world blocker" checks (so floor doesn't block placement).
		PreviewActor->SetPlacementWorldIgnoreActor(Hit.GetActor());

		// First: evaluate world collision blockers + overlaps with other objects.
		const bool bValidByWorld = PreviewActor->EvaluatePlacementAt(CandidateTransform);

		// Second: check grid occupancy footprint.
		TArray<FIntPoint> Cells;
		GetFootprintCellsForTransform(PreviewActor, CandidateTransform, Cells);
		const bool bValidByGrid = !AreAnyCellsOccupied(Cells);

		PreviewActor->SetActorTransform(CandidateTransform);
		if (!bValidByWorld)
		{
			// EvaluatePlacementAt already set PlacementState (object/world).
			PreviewActor->SetPlacementValid(false);
			if (TheHousePlacementDebug::IsEnabled())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Placement] Invalid by world (state=%d)"), (int32)PreviewActor->PlacementState);
			}
		}
		else if (!bValidByGrid)
		{
			PreviewActor->SetPlacementState(EObjectPlacementState::OverlapsGrid);
			if (TheHousePlacementDebug::IsEnabled())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Placement] Invalid by grid (cells=%d)"), Cells.Num());
			}
		}
		else
		{
			PreviewActor->SetPlacementState(EObjectPlacementState::Valid);
		}
	}
	else if (PreviewActor)
	{
		// No hit under cursor => invalid placement.
		PreviewActor->SetPlacementState(EObjectPlacementState::OverlapsWorld);
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] Trace: no hit under cursor"));
		}
	}
}

bool ATheHousePlayerController::ValidatePlacement(const FHitResult& Hit) const
{
	const float Threshold = FMath::Clamp(PlacementGrid.MinUpNormalZ, 0.f, 1.f);
	return Hit.ImpactNormal.Z >= Threshold;
}

void ATheHousePlayerController::ConfirmPlacement()
{
	// Placement is RTS-only.
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}

	if (PlacementState != EPlacementState::Previewing || !PreviewActor)
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] Confirm ignored (state=%d preview=%s)"),
				(int32)PlacementState,
				PreviewActor ? TEXT("yes") : TEXT("no"));
		}
		return;
	}

	if (!PreviewActor->IsPlacementValid())
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] Confirm blocked: placement invalid (state=%d)"),
				(int32)PreviewActor->PlacementState);
		}
		return;
	}

	// Mark grid cells as occupied (so next placements are blocked).
	TArray<FIntPoint> Cells;
	GetFootprintCellsForTransform(PreviewActor, PreviewActor->GetActorTransform(), Cells);
	MarkCellsOccupied(Cells);

	PreviewActor->FinalizePlacement();
	if (TheHousePlacementDebug::IsEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Placement] Finalized placement. Cells marked=%d"), Cells.Num());
	}

	if (PendingStockConsumeIndex != INDEX_NONE && StoredPlaceables.IsValidIndex(PendingStockConsumeIndex))
	{
		if (PreviewActor && PreviewActor->GetClass() == StoredPlaceables[PendingStockConsumeIndex])
		{
			StoredPlaceables.RemoveAt(PendingStockConsumeIndex);
		}
	}
	PendingStockConsumeIndex = INDEX_NONE;

	PreviewActor = nullptr;
	PlacementState = EPlacementState::None;

	RefreshRTSMainWidget();
}

#pragma endregion

// =========================================================
// GRID HELPERS
// =========================================================

FVector ATheHousePlayerController::SnapWorldToGridXY(const FVector& WorldLocation) const
{
	const float Cell = FMath::Max(PlacementGrid.CellSize, 1.f);
	const FVector Rel = WorldLocation - PlacementGrid.WorldOrigin;

	const float X = FMath::GridSnap(Rel.X, Cell);
	const float Y = FMath::GridSnap(Rel.Y, Cell);

	return FVector(X, Y, Rel.Z) + PlacementGrid.WorldOrigin;
}

FIntPoint ATheHousePlayerController::WorldToGridCell(const FVector& WorldLocation) const
{
	const float Cell = FMath::Max(PlacementGrid.CellSize, 1.f);
	const FVector Rel = WorldLocation - PlacementGrid.WorldOrigin;
	return FIntPoint(
		FMath::FloorToInt(Rel.X / Cell),
		FMath::FloorToInt(Rel.Y / Cell)
	);
}

void ATheHousePlayerController::GetFootprintCellsForTransform(const ATheHouseObject* Obj, const FTransform& WorldTransform, TArray<FIntPoint>& OutCells) const
{
	OutCells.Reset();
	if (!Obj || PlacementGrid.CellSize <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Approx footprint as the object's placement exclusion box projected on XY.
	const FBox WorldBox = Obj->GetLocalPlacementExclusionBox().TransformBy(WorldTransform);

	const FVector Min = WorldBox.Min;
	const FVector Max = WorldBox.Max;

	const FIntPoint MinCell = WorldToGridCell(Min);
	const FIntPoint MaxCell = WorldToGridCell(Max);

	const int32 X0 = FMath::Min(MinCell.X, MaxCell.X);
	const int32 X1 = FMath::Max(MinCell.X, MaxCell.X);
	const int32 Y0 = FMath::Min(MinCell.Y, MaxCell.Y);
	const int32 Y1 = FMath::Max(MinCell.Y, MaxCell.Y);

	OutCells.Reserve((X1 - X0 + 1) * (Y1 - Y0 + 1));
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		for (int32 X = X0; X <= X1; ++X)
		{
			OutCells.Add(FIntPoint(X, Y));
		}
	}
}

bool ATheHousePlayerController::AreAnyCellsOccupied(const TArray<FIntPoint>& Cells) const
{
	for (const FIntPoint& Cell : Cells)
	{
		if (OccupiedGridCells.Contains(Cell))
		{
			return true;
		}
	}
	return false;
}

void ATheHousePlayerController::MarkCellsOccupied(const TArray<FIntPoint>& Cells)
{
	for (const FIntPoint& Cell : Cells)
	{
		OccupiedGridCells.Add(Cell);
	}
}

void ATheHousePlayerController::MarkCellsFree(const TArray<FIntPoint>& Cells)
{
	for (const FIntPoint& Cell : Cells)
	{
		OccupiedGridCells.Remove(Cell);
	}
}

void ATheHousePlayerController::EnsureDefaultRTSContextMenuDefs()
{
	RTSContextMenuOptionDefs.RemoveAll([](const FTheHouseRTSContextMenuOptionDef& D) {
		return D.OptionId.IsNone();
	});

	if (RTSContextMenuOptionDefs.Num() > 0)
	{
		return;
	}

	{
		FTheHouseRTSContextMenuOptionDef D;
		D.OptionId = TheHouseRTSContextIds::DeleteObject;
		D.Label = NSLOCTEXT("TheHouse", "RTS_RemoveObject", "Vendre");
		RTSContextMenuOptionDefs.Add(D);
	}
	{
		FTheHouseRTSContextMenuOptionDef D;
		D.OptionId = TheHouseRTSContextIds::StoreObject;
		D.Label = NSLOCTEXT("TheHouse", "RTS_StoreObject", "Mettre en stock");
		RTSContextMenuOptionDefs.Add(D);
	}
}

void ATheHousePlayerController::InitializeModeWidgets()
{
	if (!RTSMainWidgetClass)
	{
		RTSMainWidgetClass = UTheHouseRTSMainWidget::StaticClass();
	}
	if (!RTSContextMenuWidgetClass)
	{
		RTSContextMenuWidgetClass = UTheHouseRTSContextMenuWidget::StaticClass();
	}
	if (!FPSHudWidgetClass)
	{
		FPSHudWidgetClass = UTheHouseFPSHudWidget::StaticClass();
	}

	if (RTSMainWidgetClass && !RTSMainWidgetInstance)
	{
		RTSMainWidgetInstance = CreateWidget<UTheHouseRTSMainWidget>(this, RTSMainWidgetClass);
		if (RTSMainWidgetInstance)
		{
			RTSMainWidgetInstance->BindToPlayerController(this);
			RTSMainWidgetInstance->AddToViewport(10);
		}
	}

	if (FPSHudWidgetClass && !FPSHudWidgetInstance)
	{
		FPSHudWidgetInstance = CreateWidget<UTheHouseFPSHudWidget>(this, FPSHudWidgetClass);
		if (FPSHudWidgetInstance)
		{
			FPSHudWidgetInstance->AddToViewport(5);
		}
	}

	UpdateModeWidgetsVisibility();
}

void ATheHousePlayerController::UpdateModeWidgetsVisibility()
{
	const bool bRTS = IsValid(RTSPawnReference) && (GetPawn() == RTSPawnReference);

	if (RTSMainWidgetInstance)
	{
		RTSMainWidgetInstance->SetVisibility(bRTS ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (FPSHudWidgetInstance)
	{
		FPSHudWidgetInstance->SetVisibility(
			bRTS ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

void ATheHousePlayerController::CloseRTSContextMenu()
{
	if (RTSContextMenuInstance)
	{
		RTSContextMenuInstance->RemoveFromParent();
		RTSContextMenuInstance = nullptr;
	}
}

bool ATheHousePlayerController::IsRtsCameraRotateModifierPhysicallyDown() const
{
	return TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::LeftAlt) ||
		   TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::RightAlt);
}

ATheHouseObject* ATheHousePlayerController::FindPlacedTheHouseObjectUnderCursor() const
{
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (ATheHouseObject* Obj = Cast<ATheHouseObject>(Hit.GetActor()))
		{
			if (Obj != PreviewActor && !Obj->IsPlacementPreviewActor())
			{
				return Obj;
			}
		}
	}

	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector End = WorldOrigin + WorldDir * 200000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TheHouseRTSContextPick), false);
	if (RTSPawnReference)
	{
		Params.AddIgnoredActor(RTSPawnReference);
	}
	if (PreviewActor)
	{
		Params.AddIgnoredActor(PreviewActor);
	}

	auto PickClosestTheHouseObject = [this](const TArray<FHitResult>& Hits) -> ATheHouseObject*
	{
		ATheHouseObject* Best = nullptr;
		float BestDist = TNumericLimits<float>::Max();
		for (const FHitResult& H : Hits)
		{
			ATheHouseObject* Obj = Cast<ATheHouseObject>(H.GetActor());
			if (!IsValid(Obj) || Obj == PreviewActor || Obj->IsPlacementPreviewActor())
			{
				continue;
			}
			const float D = H.Distance;
			if (D < BestDist)
			{
				BestDist = D;
				Best = Obj;
			}
		}
		return Best;
	};

	TArray<FHitResult> Hits;
	if (World->LineTraceMultiByChannel(Hits, WorldOrigin, End, ECC_Visibility, Params))
	{
		if (ATheHouseObject* Picked = PickClosestTheHouseObject(Hits))
		{
			return Picked;
		}
	}

	Hits.Reset();
	if (World->LineTraceMultiByChannel(Hits, WorldOrigin, End, ECC_WorldDynamic, Params))
	{
		if (ATheHouseObject* Picked = PickClosestTheHouseObject(Hits))
		{
			return Picked;
		}
	}

	Hits.Reset();
	if (World->LineTraceMultiByChannel(Hits, WorldOrigin, End, ECC_WorldStatic, Params))
	{
		if (ATheHouseObject* Picked = PickClosestTheHouseObject(Hits))
		{
			return Picked;
		}
	}

	// Meshes / BP qui n’ignorent pas les canaux ci-dessus mais ne bloquent pas Visibility.
	Hits.Reset();
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	if (World->LineTraceMultiByObjectType(Hits, WorldOrigin, End, ObjParams, Params))
	{
		if (ATheHouseObject* Picked = PickClosestTheHouseObject(Hits))
		{
			return Picked;
		}
	}

	return nullptr;
}

void ATheHousePlayerController::TryOpenRTSContextMenuAtCursor()
{
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}
	if (IsRtsCameraRotateModifierPhysicallyDown())
	{
		return;
	}

	ATheHouseObject* Obj = FindPlacedTheHouseObjectUnderCursor();
	if (!IsValid(Obj))
	{
		return;
	}

	if (UWorld* W = GetWorld())
	{
		const double T = W->GetTimeSeconds();
		if (LastRTSContextMenuOpenDebounceSeconds >= 0.0 &&
			FMath::IsNearlyEqual(T, LastRTSContextMenuOpenDebounceSeconds, 1.e-4))
		{
			return;
		}
		LastRTSContextMenuOpenDebounceSeconds = T;
	}

	CloseRTSContextMenu();

	float MouseX = 0.f;
	float MouseY = 0.f;
	if (!GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	if (!RTSContextMenuWidgetClass)
	{
		RTSContextMenuWidgetClass = UTheHouseRTSContextMenuWidget::StaticClass();
	}

	RTSContextMenuInstance = CreateWidget<UTheHouseRTSContextMenuWidget>(this, RTSContextMenuWidgetClass);
	if (!RTSContextMenuInstance)
	{
		return;
	}

	RTSContextMenuInstance->AddToViewport(200);
	RTSContextMenuInstance->OpenForTarget(this, Obj, FVector2D(MouseX, MouseY));
}

void ATheHousePlayerController::Input_CancelOrCloseRts()
{
	CloseRTSContextMenu();

	if (PlacementState == EPlacementState::Previewing)
	{
		CancelPlacement();
	}
}

void ATheHousePlayerController::Input_RTSObjectContext()
{
	TryOpenRTSContextMenuAtCursor();
}

void ATheHousePlayerController::RTS_DeletePlacedObject(ATheHouseObject* Obj)
{
	if (!IsValid(Obj) || Obj == PreviewActor)
	{
		return;
	}

	TArray<FIntPoint> Cells;
	GetFootprintCellsForTransform(Obj, Obj->GetActorTransform(), Cells);
	MarkCellsFree(Cells);

	if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Obj))
	{
		Sel->OnDeselect();
	}

	Obj->Destroy();
}

void ATheHousePlayerController::RTS_StorePlacedObject(ATheHouseObject* Obj)
{
	if (!IsValid(Obj) || Obj == PreviewActor)
	{
		return;
	}

	TArray<FIntPoint> Cells;
	GetFootprintCellsForTransform(Obj, Obj->GetActorTransform(), Cells);
	MarkCellsFree(Cells);

	if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Obj))
	{
		Sel->OnDeselect();
	}

	StoredPlaceables.Add(TSubclassOf<ATheHouseObject>(Obj->GetClass()));
	Obj->Destroy();
}

void ATheHousePlayerController::ExecuteRTSContextMenuOption(FName OptionId, ATheHouseObject* TargetObject)
{
	if (!IsValid(TargetObject) || TargetObject == PreviewActor || TargetObject->IsPlacementPreviewActor())
	{
		CloseRTSContextMenu();
		return;
	}

	if (OptionId == TheHouseRTSContextIds::DeleteObject)
	{
		RTS_DeletePlacedObject(TargetObject);
	}
	else if (OptionId == TheHouseRTSContextIds::StoreObject)
	{
		RTS_StorePlacedObject(TargetObject);
	}
	else
	{
		HandleRTSContextMenuOption(OptionId, TargetObject);
	}

	CloseRTSContextMenu();
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::RefreshRTSMainWidget()
{
	if (RTSMainWidgetInstance)
	{
		RTSMainWidgetInstance->RefreshAll();
	}
}

void ATheHousePlayerController::HandleRTSContextMenuOption_Implementation(FName OptionId, ATheHouseObject* TargetObject)
{
	(void)OptionId;
	(void)TargetObject;
}
