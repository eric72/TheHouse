// Fill out your copyright notice in the Description page of Project Settings.

#include "TheHousePlayerController.h"

#include "TheHouseGraphicsProfileSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "TheHouseObject.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "AI/TheHouseObjectSlotUserComponent.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouse/NPC/TheHouseNPCIdentity.h"
#include "TheHouse/NPC/TheHouseNPCSubsystem.h"
#include "TheHouse/NPC/TheHouseNPCEjectRegionVolume.h"
#include "TheHouse/NPC/TheHouseNPCAIController.h"
#include "UI/TheHouseFPSHudWidget.h"
#include "UI/TheHouseNPCRTSContextMenuUMGWidget.h"
#include "UI/TheHouseNPCOrderContextMenuUMGWidget.h"
#include "UI/TheHouseRTSContextMenuWidget.h"
#include "UI/TheHouseRTSContextMenuUMGWidget.h"
#include "UI/TheHouseRTSMainWidget.h"
#include "UI/TheHouseSettingsMenuWidget.h"
#include "UI/TheHouseRTSUITypes.h"
#include "TheHouse/Core/TheHouseHUD.h"
#include "TheHouse/Core/TheHouseSelectable.h"
#include "TheHouseCameraPawn.h"
#include "TheHouseFPSCharacter.h"
#include "TimerManager.h"
#include "HAL/IConsoleManager.h"
#include "Layout/WidgetPath.h"
#include "Slate/SObjectWidget.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PanelWidget.h"
#include "Components/Widget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "DrawDebugHelpers.h"
#include "Components/BoxComponent.h"

namespace TheHouseNPCOrdersInternal
{
	static ATheHouseNPCEjectRegionVolume* FindSmallestEjectVolumeContaining(UWorld* World, const FVector& WorldLocation)
	{
		if (!World)
		{
			return nullptr;
		}

		ATheHouseNPCEjectRegionVolume* Best = nullptr;
		float BestVol = BIG_NUMBER;
		for (TActorIterator<ATheHouseNPCEjectRegionVolume> It(World); It; ++It)
		{
			ATheHouseNPCEjectRegionVolume* Vol = *It;
			if (!IsValid(Vol) || !Vol->ContainsWorldLocation(WorldLocation))
			{
				continue;
			}

			float VolMetric = 0.f;
			if (UBoxComponent* B = Vol->EjectBox)
			{
				const FVector E = B->GetScaledBoxExtent();
				VolMetric = E.X * E.Y * E.Z;
			}

			if (VolMetric < BestVol)
			{
				BestVol = VolMetric;
				Best = Vol;
			}
		}
		return Best;
	}

	static bool ProjectPointToNavOrKeep(UWorld* World, const FVector& InGoal, FVector& OutGoal)
	{
		if (!World)
		{
			OutGoal = InGoal;
			return false;
		}

		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLoc;
			const FVector QueryExtent(500.f, 500.f, 1200.f);
			if (NavSys->ProjectPointToNavigation(InGoal, NavLoc, QueryExtent))
			{
				OutGoal = NavLoc.Location;
				return true;
			}
		}

		OutGoal = InGoal;
		return false;
	}
} // namespace TheHouseNPCOrdersInternal

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

/** Vrai si la chaîne parent UMG contient un nœud qui absorbe le hit-test (clic monde bloqué). */
static bool TheHouse_UmgChainHasHitTestAbsorbingVisibility(const UWidget* Leaf)
{
	if (!Leaf)
	{
		return false;
	}
	// GetParent() renvoie UPanelWidget*, pas UWidget* : on remonte via Cast<UWidget>(...).
	for (UWidget* W = const_cast<UWidget*>(Leaf); W != nullptr; W = Cast<UWidget>(W->GetParent()))
	{
		const ESlateVisibility V = W->GetVisibility();
		if (V == ESlateVisibility::Visible)
		{
			return true;
		}
	}
	return false;
}

/** Feuille UMG sous le curseur : est-elle dans l’un des panneaux RTS listés avec au moins un ancêtre Visible (hit-testable) ? */
static bool TheHouse_LeafUmgBlocksWorldForRtsPanels(
	UWidget* Leaf,
	const UTheHouseRTSMainWidget* RTSMain,
	const UUserWidget* RTSContext,
	const UUserWidget* RTSNPCContext,
	const UTheHousePlacedObjectSettingsWidget* Settings,
	const UUserWidget* RTSOrderObjContext,
	const UUserWidget* RTSOrderNpcContext,
	const UUserWidget* RTSOrderGroundContext,
	const UTheHouseSettingsMenuWidget* TheHouseSettingsMenu)
{
	if (!Leaf)
	{
		return false;
	}

	const bool bInMain =
		RTSMain && Leaf->GetTypedOuter<UTheHouseRTSMainWidget>() == RTSMain;
	const bool bInContext =
		RTSContext && Leaf->GetTypedOuter<UUserWidget>() == RTSContext;
	const bool bInNPCContext =
		RTSNPCContext && Leaf->GetTypedOuter<UUserWidget>() == RTSNPCContext;
	const bool bInSettings =
		Settings && Leaf->GetTypedOuter<UTheHousePlacedObjectSettingsWidget>() == Settings;
	const bool bInOrderObj =
		RTSOrderObjContext && Leaf->GetTypedOuter<UUserWidget>() == RTSOrderObjContext;
	const bool bInOrderNpc =
		RTSOrderNpcContext && Leaf->GetTypedOuter<UUserWidget>() == RTSOrderNpcContext;
	const bool bInOrderGround =
		RTSOrderGroundContext && Leaf->GetTypedOuter<UUserWidget>() == RTSOrderGroundContext;
	const bool bInTheHouseSettings =
		TheHouseSettingsMenu && Leaf->GetTypedOuter<UTheHouseSettingsMenuWidget>() == TheHouseSettingsMenu;

	if (!bInMain && !bInContext && !bInNPCContext && !bInSettings && !bInOrderObj && !bInOrderNpc && !bInOrderGround
		&& !bInTheHouseSettings)
	{
		return false;
	}

	return TheHouse_UmgChainHasHitTestAbsorbingVisibility(Leaf);
}

/** Slate sous la souris : SObjectWidget + hit-test UMG réel (évite IsUnderLocation sur la géométrie plein écran du WBP racine). */
static bool TheHouse_SlatePathBlocksRtsWorldSelection(
	const FWidgetPath& Path,
	const UTheHouseRTSMainWidget* RTSMain,
	const UUserWidget* RTSContext,
	const UUserWidget* RTSNPCContext,
	const UTheHousePlacedObjectSettingsWidget* Settings,
	const UUserWidget* RTSOrderObjContext,
	const UUserWidget* RTSOrderNpcContext,
	const UUserWidget* RTSOrderGroundContext,
	const UTheHouseSettingsMenuWidget* TheHouseSettingsMenu)
{
	if (!Path.IsValid() || Path.Widgets.Num() == 0)
	{
		return false;
	}

	for (int32 i = Path.Widgets.Num() - 1; i >= 0; --i)
	{
		const TSharedRef<SWidget>& SlateRef = Path.Widgets[i].Widget;
		const TSharedPtr<SWidget> S = SlateRef;
		if (!S.IsValid())
		{
			continue;
		}

		if (!S->GetTypeAsString().StartsWith(TEXT("SObjectWidget")))
		{
			continue;
		}

		const TSharedPtr<SObjectWidget> ObjWidget = StaticCastSharedPtr<SObjectWidget>(S);
		if (!ObjWidget.IsValid())
		{
			continue;
		}

		if (UUserWidget* HostWidget = ObjWidget->GetWidgetObject())
		{
			if (TheHouse_LeafUmgBlocksWorldForRtsPanels(
					HostWidget,
					RTSMain,
					RTSContext,
					RTSNPCContext,
					Settings,
					RTSOrderObjContext,
					RTSOrderNpcContext,
					RTSOrderGroundContext,
					TheHouseSettingsMenu))
			{
				return true;
			}
		}
	}
	return false;
}

/** Écart max (s) entre deux relâchements pour traiter un 2e clic quand le 2e press n’a pas démarré `bRtsSelectGestureFromWorld` (UMG au-dessus). */
static constexpr double TheHouse_RtsParamPanelSecondClickGapSeconds = 0.45;

/** Deux passages du poll au même GetTimeSeconds() monde (p.ex. même frame) → tolérance stricte. */
static constexpr double TheHouse_RTSContextMenuSameInstantEpsilon = 1.e-6;

/** Évite plusieurs TryOpen à la suite (ticks du timer ~120 Hz ; 1e-6 s ne suffisait pas). */
static constexpr double TheHouse_RTSContextMenuTryOpenMinIntervalSeconds = 0.08;

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

namespace TheHouseRTSContextDebug
{
	static TAutoConsoleVariable<int32> CVarContextDebug(
		TEXT("TheHouse.RTS.ContextMenu.Debug"),
		0,
		TEXT("0=off, 1=on-screen logs for RTS context menu open/pick"),
		ECVF_Default);

	static bool IsEnabled()
	{
		return CVarContextDebug.GetValueOnGameThread() != 0;
	}

	static void Screen(APlayerController* PC, const FString& Msg, FColor Color = FColor::Yellow)
	{
#if !UE_BUILD_SHIPPING
		if (!IsEnabled())
		{
			return;
		}
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 2.5f, Color, Msg);
		}
		if (IsValid(PC))
		{
			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		}
#endif
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
  }
  if (IsKeyPhysicallyDown(PC, EKeys::S)) {
    v -= 1.f;
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
      StaffPlacementPreviewNpc && (bShiftMod || bCtrlPivotMod))
  {
    const float Step = FMath::Clamp(PlacementRotationStepDegrees, 1.f, 180.f);
    const float Sign = FMath::Sign(WheelDelta);
    if (!FMath::IsNearlyZero(Sign))
    {
      PlacementPreviewYawDegrees =
          FMath::UnwindDegrees(PlacementPreviewYawDegrees + Sign * Step);
      UpdateStaffNpcPlacementPreview();
    }
    return;
  }

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

	TheHouse_StartInputPollTimer();

	// Live Coding / hot reload : éviter qu’un cooldown ou un poll RMB reste dans un état bloquant.
	LastRTSContextMenuTryOpenTimeSeconds = -1.0;
	LastRTSContextMenuRMBHandledTimeSeconds = -1.0;

	EnsureDefaultRTSContextMenuDefs();
	EnsureDefaultNPCRTSContextMenuDefs();
	RefreshNPCStaffRecruitmentOffers();
	TheHouse_StartRecruitmentAutoRefreshTimerIfNeeded();
	TheHouse_StartInGameClockTimerIfNeeded();
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
}

void ATheHousePlayerController::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
	if (GetWorld()) {
    GetWorldTimerManager().ClearTimer(TheHouseInputPollTimer);
	GetWorldTimerManager().ClearTimer(TheHouseRecruitmentRefreshTimer);
  }
	bTheHouseInGameClockActive = false;

  CloseRTSContextMenu();
  CloseTheHouseSettingsMenu();
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

void ATheHousePlayerController::TheHouse_StartInGameClockTimerIfNeeded()
{
	if (!GetWorld())
	{
		return;
	}

	InGameTimeSeconds = FMath::Max(0.f, InGameTimeSeconds);
	InGameDayIndex = FMath::Max(0, InGameDayIndex);

	// New game safety: avoid always starting at midnight (black scene) when no save restored the clock yet.
	if (!bTheHouseAppliedInitialClockProgress && InGameTimeSeconds <= KINDA_SMALL_NUMBER && InGameDayIndex == 0)
	{
		const float DayLen = FMath::Max(1.f, InGameDayLengthRealSeconds);
		const float InitialProgress = FMath::Clamp(InitialInGameDayProgress01, 0.f, 1.f);
		InGameTimeSeconds = DayLen * InitialProgress;
		InGameDayIndex = FMath::FloorToInt(InGameTimeSeconds / DayLen);
		bTheHouseAppliedInitialClockProgress = true;
	}

	InGameLastProcessedDayIndex = InGameDayIndex;

	// Avance fluide dans Tick (jour/nuit, barres de progression) ; pas de pas discret 0,25 s.
	bTheHouseInGameClockActive = true;
}

void ATheHousePlayerController::TheHouse_AdvanceInGameClock(float DeltaSeconds)
{
	UWorld* World = GetWorld();
	if (!World || !bTheHouseInGameClockActive)
	{
		return;
	}

	const float DayLen = FMath::Max(1.f, InGameDayLengthRealSeconds);
	InGameTimeSeconds += DeltaSeconds;

	const int32 TargetDay = FMath::Max(0, FMath::FloorToInt(InGameTimeSeconds / DayLen));
	while (InGameDayIndex < TargetDay)
	{
		++InGameDayIndex;
		TheHouse_HandleNewInGameDay();
	}

	// Si tu affiches l’horloge dans le RTS main widget via WBP (TextBlock/ProgressBar bind),
	// le refresh UI sera déclenché côté BP. Ici on ne force pas un RefreshAll chaque tick.
}

void ATheHousePlayerController::TheHouse_HandleNewInGameDay()
{
	// Dédoublonnage si jamais.
	if (InGameDayIndex == InGameLastProcessedDayIndex)
	{
		return;
	}
	InGameLastProcessedDayIndex = InGameDayIndex;

	if (!bEnableDailyStaffPayroll)
	{
		return;
	}
	if (PayrollDaysPerMonth <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UTheHouseNPCSubsystem* Registry = World->GetSubsystem<UTheHouseNPCSubsystem>();
	if (!Registry)
	{
		return;
	}

	// Pay only staff employed for >= 1 full day.
	const TArray<ATheHouseNPCCharacter*> Staff = Registry->GetNPCsByCategory(ETheHouseNPCCategory::Staff);
	const float DayLen = FMath::Max(1.f, InGameDayLengthRealSeconds);

	int32 TotalPaid = 0;
	for (ATheHouseNPCCharacter* Npc : Staff)
	{
		if (!IsValid(Npc))
		{
			continue;
		}

		// Must be employed (timestamp set) and not freshly hired today.
		if (Npc->StaffEmploymentStartInGameSeconds <= 0.f)
		{
			continue;
		}
		const int32 HireDay = FMath::FloorToInt(Npc->StaffEmploymentStartInGameSeconds / DayLen);
		if (InGameDayIndex - HireDay < 1)
		{
			continue;
		}

		if (Npc->StaffLastSalaryPaidDayIndex >= InGameDayIndex)
		{
			continue;
		}

		const float Daily = Npc->StaffMonthlySalary / float(FMath::Max(1, PayrollDaysPerMonth));
		const int32 Charge = FMath::Max(0, FMath::RoundToInt(Daily));
		if (Charge > 0)
		{
			Money = FMath::Max(0, Money - Charge);
			TotalPaid += Charge;
		}
		Npc->StaffLastSalaryPaidDayIndex = InGameDayIndex;
	}

	if (TotalPaid > 0)
	{
		RefreshRTSMainWidget();
	}
}

FText ATheHousePlayerController::GetInGameClockText() const
{
	const float DayLen = FMath::Max(1.f, InGameDayLengthRealSeconds);
	const float DayFrac = FMath::Fmod(InGameTimeSeconds, DayLen) / DayLen;
	const float Hours = FMath::Clamp(InGameHoursPerDay, 1.f, 48.f);
	const float TotalHours = DayFrac * Hours;
	const int32 H = FMath::FloorToInt(TotalHours);
	const int32 M = FMath::FloorToInt((TotalHours - float(H)) * 60.f);

	FNumberFormattingOptions TwoDigits;
	TwoDigits.MinimumIntegralDigits = 2;
	TwoDigits.MaximumIntegralDigits = 2;

	return FText::Format(
		NSLOCTEXT("TheHouse", "InGameClock", "Jour {0} · {1}:{2}"),
		FText::AsNumber(InGameDayIndex + 1),
		FText::AsNumber(H, &TwoDigits),
		FText::AsNumber(M, &TwoDigits));
}

float ATheHousePlayerController::GetInGameDayProgress01() const
{
	const float DayLen = FMath::Max(1.f, InGameDayLengthRealSeconds);
	return FMath::Fmod(InGameTimeSeconds, DayLen) / DayLen;
}

void ATheHousePlayerController::SetInGameClockState(const float NewInGameTimeSeconds, const int32 NewDayIndex)
{
	InGameTimeSeconds = FMath::Max(0.f, NewInGameTimeSeconds);
	bTheHouseAppliedInitialClockProgress = true;

	const float DayLen = FMath::Max(1.f, InGameDayLengthRealSeconds);
	const int32 DerivedDay = FMath::Max(0, FMath::FloorToInt(InGameTimeSeconds / DayLen));
	InGameDayIndex = FMath::Max(0, NewDayIndex);

	// Si la valeur passée est incohérente, on force la cohérence.
	if (InGameDayIndex != DerivedDay)
	{
		InGameDayIndex = DerivedDay;
	}

	InGameLastProcessedDayIndex = InGameDayIndex;

	// Assure que le timer tourne (utile après Load).
	TheHouse_StartInGameClockTimerIfNeeded();
}

void ATheHousePlayerController::TheHouse_StartRecruitmentAutoRefreshTimerIfNeeded()
{
	if (!GetWorld())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(TheHouseRecruitmentRefreshTimer);

	if (!bAutoRefreshNPCStaffRecruitmentRoster)
	{
		return;
	}
	if (NPCStaffRecruitmentPool.Num() == 0)
	{
		return;
	}
	const float Interval = FMath::Max(0.f, NPCStaffRecruitmentAutoRefreshIntervalSeconds);
	if (Interval <= 0.f)
	{
		return;
	}

	GetWorldTimerManager().SetTimer(
		TheHouseRecruitmentRefreshTimer,
		this,
		&ATheHousePlayerController::TheHouse_OnRecruitmentAutoRefreshTimer,
		Interval,
		/*bLoop*/ true);
}

void ATheHousePlayerController::TheHouse_OnRecruitmentAutoRefreshTimer()
{
	// Refresh “vivant” : si le pool existe, régénère la liste entière (et donc les stats, noms, étoiles, etc.)
	// La conso d’une offre au hiring est gérée dans ConfirmStaffNpcPlacement.
	if (NPCStaffRecruitmentPool.Num() == 0)
	{
		return;
	}
	RefreshNPCStaffRecruitmentOffers();
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
  // If a BP overrides BeginPlay without calling Parent, the in-game clock may never get activated.
  if (GetWorld() && !bTheHouseInGameClockActive) {
    TheHouse_StartInGameClockTimerIfNeeded();
  }
  TheHouse_AdvanceInGameClock(DeltaTime);
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

  // Safety: if we somehow got stuck in UIOnly, restore normal RTS input mode
  // when no context menu is open.
  if (Cast<ATheHouseCameraPawn>(GetPawn()) == RTSPawnReference && RTSPawnReference &&
      !bIsRotateModifierDown && !AnyRTSContextMenuOpen())
  {
    bShowMouseCursor = true;
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    InputMode.SetHideCursorDuringCapture(false);
    SetInputMode(InputMode);
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

  // Menu contextuel RTS : en Game+UI, PlayerInput peut ne pas voir RMB ; on utilise
  // l’état physique (poll). Ne PAS aussi binder RTSObjectContext sur RMB : sinon le
  // même front montant ouvre via BindAction puis ce bloc voit le menu ouvert et le
  // ferme (toggle) dans la même frame → fenêtre invisible.
  //
  // Ne pas utiliser WasInputKeyJustPressed ici : le poll peut tourner plusieurs fois
  // par frame (120 Hz) et le fallback re-déclenche ouverture/fermeture en rafale.
  // Un seul traitement du front montant par « instant » monde (sans GFrameCounter — UE 5.7+).
  {
		// Physique + PlayerInput : avec Enhanced Input / focus, la touche « physique » seule peut rester à faux.
		const bool bPhysRMB = TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::RightMouseButton);
		const bool bKeyRMB = IsInputKeyDown(EKeys::RightMouseButton);
		const bool bRMB = bPhysRMB || bKeyRMB;
		const bool bRising = (!bPrevPollRMBHeld && bRMB);

		if (bRising &&
			Cast<ATheHouseCameraPawn>(GetPawn()) != nullptr &&
			!IsRtsCameraRotateModifierPhysicallyDown())
    {
      const double NowSec = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
      const bool bDuplicateSameInstant =
          LastRTSContextMenuRMBHandledTimeSeconds >= 0.0 &&
          FMath::IsNearlyEqual(NowSec, LastRTSContextMenuRMBHandledTimeSeconds, TheHouse_RTSContextMenuSameInstantEpsilon);
      if (!bDuplicateSameInstant)
      {
        LastRTSContextMenuRMBHandledTimeSeconds = NowSec;
        // Toggle: if already open, close; otherwise open.
        if (AnyRTSContextMenuOpen())
        {
          CloseRTSContextMenu();
        }
        else
        {
          TryOpenRTSContextMenuAtCursor();
        }
      }
    }
    else if (bRising && TheHouseRTSContextDebug::IsEnabled())
    {
      TheHouseRTSContextDebug::Screen(
        this,
        FString::Printf(TEXT("[RTSContext] RMB detected but ignored | RTS cam=%s | Alt=%s | Pawn=%s"),
          Cast<ATheHouseCameraPawn>(GetPawn()) ? TEXT("yes") : TEXT("no"),
          IsRtsCameraRotateModifierPhysicallyDown() ? TEXT("yes") : TEXT("no"),
          GetPawn() ? *GetPawn()->GetName() : TEXT("null")),
        FColor::Silver);
    }
    bPrevPollRMBHeld = bRMB;
  }

  // Clic gauche RTS : en Game+UI, le même symptôme que RMB (seuls Échap / chemins hors BindAction semblaient fiables).
  {
    const bool bPhysLMB = TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::LeftMouseButton);
    const bool bKeyLMB = IsInputKeyDown(EKeys::LeftMouseButton);
    const bool bLMB = bPhysLMB || bKeyLMB;
    const bool bLMBRising = (!bPrevPollLMBHeld && bLMB);
    const bool bLMBFalling = (bPrevPollLMBHeld && !bLMB);

    if (Cast<ATheHouseCameraPawn>(GetPawn()) != nullptr && !IsRtsCameraRotateModifierPhysicallyDown())
    {
      if (bLMBRising)
      {
        Input_SelectPressed();
      }
      if (bLMBFalling)
      {
        Input_SelectReleased();
      }
    }
    bPrevPollLMBHeld = bLMB;
  }
}

void ATheHousePlayerController::SetupInputComponent() {
  // Le parent crée InputComponent depuis DefaultInputComponentClass (souvent Enhanced si l’INI a été réécrit).
  APlayerController::SetupInputComponent();

  // Les BindAction legacy (« Select », etc.) exigent UPlayerInput + UInputComponent.
  if (InputComponent && InputComponent->IsA(UEnhancedInputComponent::StaticClass()))
  {
#if !UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Warning,
      TEXT("[TheHouse] PlayerController : remplacement de %s par UInputComponent (vérifie DefaultInput.ini : DefaultInputComponentClass)."),
      *InputComponent->GetClass()->GetName());
#endif
    InputComponent->UnregisterComponent();
    InputComponent->DestroyComponent();
    InputComponent = nullptr;
  }

  if (InputComponent == nullptr)
  {
    InputComponent = NewObject<UInputComponent>(
        this, UInputComponent::StaticClass(), TEXT("PC_InputComponent0"));
    InputComponent->RegisterComponent();
  }

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
      "TheHouseToggleGraphicsProfile",
      IE_Pressed,
      this,
      &ATheHousePlayerController::Input_ToggleGraphicsProfile);
  InputComponent->BindAction(
      "TheHouseOpenSettings",
      IE_Pressed,
      this,
      &ATheHousePlayerController::Input_TheHouseOpenSettings);
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
  {
    FInputActionBinding& Press = InputComponent->BindAction(
      "Select",
      IE_Pressed,
      this,
      &ATheHousePlayerController::Input_SelectPressed);
    Press.bConsumeInput = false; // let Slate/UMG receive the click too

    FInputActionBinding& Rel = InputComponent->BindAction(
      "Select",
      IE_Released,
      this,
      &ATheHousePlayerController::Input_SelectReleased);
    Rel.bConsumeInput = false;
  }

  // Double click (Left mouse) : ouvrir le panneau paramètres même si le 2e press est absorbé par l’UMG.
  {
    FInputKeyBinding& Dbl = InputComponent->BindKey(
      EKeys::LeftMouseButton,
      IE_DoubleClick,
      this,
      &ATheHousePlayerController::Input_SelectDoubleClick);
    Dbl.bConsumeInput = false;
  }

  InputComponent->BindAction(
      "CancelOrCloseRts",
      IE_Pressed,
      this,
      &ATheHousePlayerController::Input_CancelOrCloseRts);

  // FPS : saut / course / accroupi — noms d’actions = Project Settings → Input
  // (TheHouseFPS*) ; remappable aussi via UInputSettings en jeu (écran options).
  {
    InputComponent->BindAction(TEXT("TheHouseFPSJump"), IE_Pressed, this,
                               &ATheHousePlayerController::Input_FPSJumpPressed);
    InputComponent->BindAction(TEXT("TheHouseFPSJump"), IE_Released, this,
                               &ATheHousePlayerController::Input_FPSJumpReleased);
    InputComponent->BindAction(TEXT("TheHouseFPSRun"), IE_Pressed, this,
                               &ATheHousePlayerController::Input_FPSRunPressed);
    InputComponent->BindAction(TEXT("TheHouseFPSRun"), IE_Released, this,
                               &ATheHousePlayerController::Input_FPSRunReleased);
    InputComponent->BindAction(TEXT("TheHouseFPS_Crouch"), IE_Pressed, this,
                               &ATheHousePlayerController::Input_FPSCrouchPressed);
    InputComponent->BindAction(TEXT("TheHouseFPS_Crouch"), IE_Released, this,
                               &ATheHousePlayerController::Input_FPSCrouchReleased);

    InputComponent->BindAction(TEXT("TheHouseFPSFire"), IE_Pressed, this,
                               &ATheHousePlayerController::Input_FPSFirePressed);
  }

  // RTSObjectContext (RMB) : volontairement NON bindé ici — géré uniquement par
  // TheHouse_PollInputFrame (voir commentaire au-dessus du bloc RMB).


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

	if (!CanAffordCatalogPurchaseForClass(PendingPlacementClass))
	{
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] P ignored: pas assez d'argent pour %s"), *GetNameSafe(PendingPlacementClass));
		}
		return;
	}

	RTS_DeselectAll();
	SpawnPlacementPreviewFromPendingClass();
}

void ATheHousePlayerController::CancelPlacement()
{
	if (StaffPlacementPreviewNpc)
	{
		StaffPlacementPreviewNpc->Destroy();
		StaffPlacementPreviewNpc = nullptr;
	}
	PendingStaffSpawnOffer = FTheHouseNPCStaffRosterOffer();
	bStaffNpcPlacementLocationValid = false;

	if (PreviewActor)
	{
		if (bRelocationPreviewActive)
		{
			PreviewActor->SetActorTransform(RelocationRestoreTransform);
			TArray<FIntPoint> Cells;
			GetFootprintCellsForTransform(PreviewActor, PreviewActor->GetActorTransform(), Cells);
			MarkCellsOccupied(Cells);
			PreviewActor->FinalizePlacement();
			PreviewActor = nullptr;
		}
		else
		{
			PreviewActor->TeardownPlacementBlockerVisualFeedback();
			PreviewActor->Destroy();
			PreviewActor = nullptr;
		}
	}
	bRelocationPreviewActive = false;
	PlacementState = EPlacementState::None;
	PlacementPreviewYawDegrees = 0.f;
	PendingStockConsumeIndex = INDEX_NONE;
}

void ATheHousePlayerController::RTS_DeselectAll(bool bClosePlacedObjectSettings)
{
	// Désélection RTS = reset gameplay ; l'appelant décide si les UI modales doivent fermer.
	// Par défaut on ferme le panneau paramètres, mais les menus contextuels doivent aussi partir
	// quand on "clique dans le vide" (la plupart des appels passent par bClosePlacedObjectSettings=true).
	CloseRtsModalUi(/*bClosePlacedObjectSettings=*/bClosePlacedObjectSettings);

	for (TWeakObjectPtr<AActor>& W : RTSSelectedActors)
	{
		if (AActor* A = W.Get())
		{
			if (ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(A))
			{
				if (ATheHouseNPCAIController* HouseAI = Cast<ATheHouseNPCAIController>(Npc->GetController()))
				{
					HouseAI->ClearScriptedAttackOrder();
				}
			}
			if (ITheHouseSelectable* S = Cast<ITheHouseSelectable>(A))
			{
				S->OnDeselect();
			}
		}
	}
	RTSSelectedActors.Reset();
	LastRtsParamPanelObjectReleaseTimeSeconds = -1.0;
	LastRtsParamPanelObjectRelease.Reset();
}

void ATheHousePlayerController::CloseRtsModalUi(bool bClosePlacedObjectSettings)
{
	CloseRTSContextMenu();
	CloseTheHouseSettingsMenu();
	if (bClosePlacedObjectSettings)
	{
		ClosePlacedObjectSettingsWidget();
	}
}

void ATheHousePlayerController::CancelScriptedGuardAttackOnSelectedNPCs()
{
	for (const TWeakObjectPtr<AActor>& W : RTSSelectedActors)
	{
		if (ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(W.Get()))
		{
			if (ATheHouseNPCAIController* HouseAI = Cast<ATheHouseNPCAIController>(Npc->GetController()))
			{
				HouseAI->ClearScriptedAttackOrder();
			}
		}
	}
}

void ATheHousePlayerController::RTS_SelectExclusive(AActor* Actor)
{
	RTS_DeselectAll(/*bClosePlacedObjectSettings=*/false);
	if (!IsValid(Actor) || !Actor->Implements<UTheHouseSelectable>())
	{
		ClosePlacedObjectSettingsWidget();
		return;
	}
	ITheHouseSelectable* S = Cast<ITheHouseSelectable>(Actor);
	if (!S || !S->IsSelectable())
	{
		ClosePlacedObjectSettingsWidget();
		return;
	}
	S->OnSelect();
	RTSSelectedActors.Add(Actor);
}

void ATheHousePlayerController::RTS_SelectFromBox(const TArray<AActor*>& CandidateActors)
{
	RTS_DeselectAll();
	for (AActor* A : CandidateActors)
	{
		if (!IsValid(A) || !A->Implements<UTheHouseSelectable>())
		{
			continue;
		}
		ITheHouseSelectable* S = Cast<ITheHouseSelectable>(A);
		if (!S || !S->IsSelectable())
		{
			continue;
		}
		S->OnSelect();
		RTSSelectedActors.AddUnique(A);
	}
}

void ATheHousePlayerController::SpawnPlacementPreviewFromPendingClass()
{
	if (!GetWorld() || !PendingPlacementClass)
	{
		return;
	}

	RTS_DeselectAll();

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
	if (!CanAffordCatalogPurchaseForClass(InClass))
	{
		return;
	}

	PendingStockConsumeIndex = INDEX_NONE;
	PendingPlacementClass = InClass;
	CancelPlacement();
	SpawnPlacementPreviewFromPendingClass();
}

void ATheHousePlayerController::AddOrIncrementStoredStock(TSubclassOf<ATheHouseObject> ClassToStore)
{
	if (!ClassToStore)
	{
		return;
	}
	for (FTheHouseStoredStack& Stack : StoredPlaceableStacks)
	{
		if (Stack.ObjectClass == ClassToStore)
		{
			++Stack.Count;
			return;
		}
	}
	FTheHouseStoredStack NewStack;
	NewStack.ObjectClass = ClassToStore;
	NewStack.Count = 1;
	StoredPlaceableStacks.Add(NewStack);
}

void ATheHousePlayerController::ConsumeStoredPlaceableAndBeginPreview(int32 StoredStackIndex)
{
	if (!StoredPlaceableStacks.IsValidIndex(StoredStackIndex))
	{
		return;
	}
	const FTheHouseStoredStack& Stack = StoredPlaceableStacks[StoredStackIndex];
	if (!Stack.ObjectClass || Stack.Count <= 0)
	{
		return;
	}

	PendingPlacementClass = Stack.ObjectClass;
	CancelPlacement();
	PendingStockConsumeIndex = StoredStackIndex;
	SpawnPlacementPreviewFromPendingClass();
}

void ATheHousePlayerController::StartRelocationPreviewForPlacedObject(ATheHouseObject* PlacedObject)
{
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}
	if (!IsValid(PlacedObject) || PlacedObject->IsPlacementPreviewActor())
	{
		return;
	}

	CancelPlacement();

	RTSSelectedActors.RemoveAllSwap([&](const TWeakObjectPtr<AActor>& W)
	{
		return W.Get() == PlacedObject;
	});

	RelocationRestoreTransform = PlacedObject->GetActorTransform();
	TArray<FIntPoint> OldCells;
	GetFootprintCellsForTransform(PlacedObject, RelocationRestoreTransform, OldCells);
	MarkCellsFree(OldCells);

	PreviewActor = PlacedObject;
	PendingStockConsumeIndex = INDEX_NONE;
	PendingPlacementClass = TSubclassOf<ATheHouseObject>(PlacedObject->GetClass());

	PlacementPreviewYawDegrees = PlacedObject->GetActorRotation().Yaw;

	PreviewActor->SetAsPreview(true);
	PlacementState = EPlacementState::Previewing;
	bRelocationPreviewActive = true;

	if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(PlacedObject))
	{
		Sel->OnDeselect();
	}
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
      // FPS : même repère que MoveForward (yaw contrôleur) pour que strafe suive la visée
      // même si le corps pivote en retard (bDelayBodyYawUntilLookSideways).
      if (Cast<ATheHouseFPSCharacter>(Char))
      {
        const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
        const FVector RtDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
        Char->AddMovementInput(RtDir, Value, true);
      }
      else
      {
        Char->AddMovementInput(Char->GetActorRightVector(), Value, true);
      }
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
  // Zoom symétrique : appliquer un changement relatif (multiplicatif) pour que
  // zoom-in puis zoom-out avec le même input reviennent au même arm length.
  // On conserve RTSZoomSpeed comme "intensité" via un delta équivalent à l'ancienne formule.
  const float DeltaLen = (ScaledValue * RTSZoomSpeed * -1.f);
  const float Mult = FMath::Exp(DeltaLen / FMath::Max(OldLength, 1.f));
  const float NewLength = FMath::Clamp(OldLength * Mult, MinL, MaxL);
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

void ATheHousePlayerController::Input_FPSJumpPressed()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->Jump();
	}
}

void ATheHousePlayerController::Input_FPSJumpReleased()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->StopJumping();
	}
}

void ATheHousePlayerController::Input_FPSRunPressed()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->SetFPSMovementSprinting(true);
	}
}

void ATheHousePlayerController::Input_FPSRunReleased()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->SetFPSMovementSprinting(false);
	}
}

void ATheHousePlayerController::Input_FPSCrouchPressed()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->Crouch();
	}
}

void ATheHousePlayerController::Input_FPSCrouchReleased()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->UnCrouch();
	}
}

void ATheHousePlayerController::Input_FPSFirePressed()
{
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(GetPawn()))
	{
		Fps->FireWeaponOnce();
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
  ClosePlacedObjectSettingsWidget();
  UpdateModeWidgetsVisibility();
}

void ATheHousePlayerController::SwitchToFPS(FVector TargetLocation, FRotator TargetRotation) {
	if (!FPSCharacterClass) {
		FPSCharacterClass = ATheHouseFPSCharacter::StaticClass();
	}

	// Placement preview is RTS-only.
	CancelPlacement();
	// Quitter le mode RTS => toujours enlever la sélection RTS (sinon overlay / custom depth restent).
	RTS_DeselectAll(/*bClosePlacedObjectSettings=*/true);
	CloseRTSContextMenu();
	ClosePlacedObjectSettingsWidget();

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

		// Capsule-only world collision (meshes visible for anim tests but do not block traces / camera).
		ActiveFPSCharacter->ApplyFirstPersonCapsuleOnlyCollision();
		ActiveFPSCharacter->RefreshFPSCameraAttachment();

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

void ATheHousePlayerController::Input_ToggleGraphicsProfile()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTheHouseGraphicsProfileSubsystem* Sub = GI->GetSubsystem<UTheHouseGraphicsProfileSubsystem>())
		{
			Sub->ToggleProfile();
		}
	}
}

void ATheHousePlayerController::TheHouse_SetGraphicsProfile(int32 ProfileIndex)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTheHouseGraphicsProfileSubsystem* Sub = GI->GetSubsystem<UTheHouseGraphicsProfileSubsystem>())
		{
			Sub->SetProfile(ProfileIndex != 0 ? ETheHouseGraphicsProfile::High : ETheHouseGraphicsProfile::Low);
		}
	}
}


void ATheHousePlayerController::Input_SelectPressed() {
  if (Cast<ATheHouseCameraPawn>(GetPawn()) != nullptr)
  {
    if (LastRtsLmbSelectPressFrame == GFrameCounter)
    {
      return;
    }
    LastRtsLmbSelectPressFrame = GFrameCounter;
  }

  bRtsSelectGestureFromWorld = false;

  // Si le menu contextuel est ouvert, on laisse l'UI recevoir le clic (Slate).
  // Ne PAS fermer ici : sinon le bouton ne reçoit jamais l'événement.
  if (AnyRTSContextMenuOpen())
  {
    return;
  }
  // While rotating (ALT / RMB), left click should not start selection nor confirm placement.
  if (bIsRotateModifierDown) {
    return;
  }

  if (PlacementState == EPlacementState::Previewing)
  {
    if (StaffPlacementPreviewNpc)
    {
      ConfirmStaffNpcPlacement();
    }
    else if (PreviewActor)
    {
      ConfirmPlacement();
    }
    return;
  }
  if (!RTSPawnReference || GetPawn() != RTSPawnReference)
    return;

  // Menus contextuels / panneau paramètres uniquement — pas le Slate « chrome » du HUD RTS (sinon aucun clic monde).
  if (TheHouse_IsModalRtsUiBlockingWorldSelection())
  {
    return;
  }

  // Start Selection
  float MouseX, MouseY;
  if (GetMousePosition(MouseX, MouseY)) {
    bRtsSuppressNextSelectReleaseWorldPick = false;
    bRtsSelectGestureFromWorld = true;
    bIsSelecting = true;
    SelectionStartPos = FVector2D(MouseX, MouseY);

    // Notify HUD
    if (ATheHouseHUD *HUD = Cast<ATheHouseHUD>(GetHUD())) {
      HUD->StartSelection(SelectionStartPos);
    }
  }
}

void ATheHousePlayerController::Input_SelectReleased() {
  if (Cast<ATheHouseCameraPawn>(GetPawn()) != nullptr)
  {
    if (LastRtsLmbSelectReleaseFrame == GFrameCounter)
    {
      return;
    }
    LastRtsLmbSelectReleaseFrame = GFrameCounter;
  }

  const bool bGestureFromWorld = bRtsSelectGestureFromWorld;
  bRtsSelectGestureFromWorld = false;

  // If context menu is open, do not run selection logic.
  if (AnyRTSContextMenuOpen())
  {
	// Clic gauche en dehors des menus modaux : fermer les menus (même comportement que RMB toggle),
	// sinon laisser l'UMG recevoir le release (ne pas fermer quand le curseur est sur le menu).
	if (!TheHouse_IsModalRtsUiBlockingWorldSelection())
	{
		CloseRtsModalUi(/*bClosePlacedObjectSettings=*/true);
	}
    bIsSelecting = false;
    if (ATheHouseHUD* HUD = Cast<ATheHouseHUD>(GetHUD()))
    {
      HUD->StopSelection();
    }
    return;
  }
  // While rotating (ALT / RMB), ignore selection release too.
  if (bIsRotateModifierDown) {
    bIsSelecting = false;
    if (ATheHouseHUD* HUD = Cast<ATheHouseHUD>(GetHUD()))
    {
      HUD->StopSelection();
    }
    return;
  }

  bIsSelecting = false;

  // Notify HUD to stop drawing
  if (ATheHouseHUD *HUD = Cast<ATheHouseHUD>(GetHUD())) {
    HUD->StopSelection();
  }

  if (bRtsSuppressNextSelectReleaseWorldPick)
  {
    bRtsSuppressNextSelectReleaseWorldPick = false;
    return;
  }

  if (!IsValid(RTSPawnReference) || GetPawn() != RTSPawnReference)
    return;

  float MouseX, MouseY;
  if (!GetMousePosition(MouseX, MouseY))
    return;

  FVector2D CurrentMousePos(MouseX, MouseY);
  // Sans drag HUD « monde » (press sur UMG, etc.) : traiter comme clic ponctuel pour pouvoir désélectionner
  // au relâchement sur la carte (sinon le return bloquait tout le bloc ci‑dessous).
  const float DragDistance =
      bGestureFromWorld ? FVector2D::Distance(SelectionStartPos, CurrentMousePos) : 0.f;

  // Exception : 2e relâchement rapide sur le même objet casino alors que le 2e press a été mangé par l’UMG.
  if (!bGestureFromWorld)
  {
    if (PlacementState != EPlacementState::Previewing && PlacedObjectSettingsWidgetClass)
    {
      if (UWorld* World = GetWorld())
      {
        FHitResult Hit;
        if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
        {
          if (ATheHouseObject* Obj = Cast<ATheHouseObject>(Hit.GetActor()))
          {
            if (Obj->bOpenParametersPanelOnLeftClick && !Obj->IsPlacementPreviewActor() &&
                Obj->Implements<UTheHouseSelectable>())
            {
              if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Obj))
              {
                if (Sel->IsSelectable())
                {
                  const double Now = World->GetTimeSeconds();
                  if (LastRtsParamPanelObjectRelease.Get() == Obj &&
                      LastRtsParamPanelObjectReleaseTimeSeconds >= 0.0 &&
                      (Now - LastRtsParamPanelObjectReleaseTimeSeconds) <= TheHouse_RtsParamPanelSecondClickGapSeconds)
                  {
                    RTS_SelectExclusive(Obj);
                    OpenPlacedObjectSettingsWidgetFor(Obj);
                    LastRtsParamPanelObjectReleaseTimeSeconds = Now;
                    LastRtsParamPanelObjectRelease = Obj;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  // Log for debug
  // UE_LOG(LogTemp, Log, TEXT("Selection Released. Drag Dist: %f"),
  // DragDistance);

  if (DragDistance < 10.0f) // Small movement = Single Click
  {
    // Traces monde (pas Slate) : un panneau UMG plein écran hit-testable bloquait avant tout le bloc,
    // donc un clic « sur la carte » ne désélectionnait jamais tant que le panneau paramètres était ouvert.
    AActor* const HitActor = FindPrimarySelectableActorUnderCursorRTS();

    if (HitActor)
    {
      if (TheHouse_IsModalRtsUiBlockingWorldSelection())
      {
        return;
      }

      RTS_SelectExclusive(HitActor);
      if (ATheHouseObject* PlacedObj = Cast<ATheHouseObject>(HitActor))
      {
        OpenPlacedObjectSettingsWidgetFor(PlacedObj);
        if (PlacedObj->bOpenParametersPanelOnLeftClick && PlacedObjectSettingsWidgetClass)
        {
          if (UWorld* World = GetWorld())
          {
            LastRtsParamPanelObjectReleaseTimeSeconds = World->GetTimeSeconds();
            LastRtsParamPanelObjectRelease = PlacedObj;
          }
        }
        if (bOpenContextMenuWhenLeftClickObject)
        {
          TryOpenRTSContextMenuForObject(PlacedObj);
        }
      }
      else
      {
        ClosePlacedObjectSettingsWidget();
      }
    }
    else
    {
      RTS_DeselectAll();
    }
  } else // Box Selection
  {
    TArray<AActor *> SelectedActors;

    if (AHUD *HUD = GetHUD()) {
      HUD->GetActorsInSelectionRectangle<AActor>(
          SelectionStartPos, CurrentMousePos, SelectedActors, false, false);
    }

	AppendSelectableNPCsIntersectingRTSBox(SelectionStartPos, CurrentMousePos, SelectedActors);

	RTS_SelectFromBox(SelectedActors);
  }
}

void ATheHousePlayerController::Input_SelectDoubleClick()
{
	// Même garde-fous que la sélection classique.
	if (AnyRTSContextMenuOpen() || bIsRotateModifierDown)
	{
		return;
	}
	if (PlacementState == EPlacementState::Previewing)
	{
		if (StaffPlacementPreviewNpc)
		{
			ConfirmStaffNpcPlacement();
		}
		else if (PreviewActor)
		{
			ConfirmPlacement();
		}
		return;
	}
	if (!IsValid(RTSPawnReference) || GetPawn() != RTSPawnReference)
	{
		return;
	}
	if (TheHouse_IsModalRtsUiBlockingWorldSelection())
	{
		return;
	}

	FHitResult Hit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		return;
	}
	ATheHouseObject* Obj = Cast<ATheHouseObject>(Hit.GetActor());
	if (!IsValid(Obj) || Obj->IsPlacementPreviewActor())
	{
		return;
	}
	if (!Obj->bOpenParametersPanelOnLeftClick || !PlacedObjectSettingsWidgetClass)
	{
		return;
	}
	if (!Obj->Implements<UTheHouseSelectable>())
	{
		return;
	}
	if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Obj))
	{
		if (!Sel->IsSelectable())
		{
			return;
		}
	}

	RTS_SelectExclusive(Obj);
	OpenPlacedObjectSettingsWidgetFor(Obj);
}


// Object placemet on the map

void ATheHousePlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);

	if (GetWorld() && GetPawn() == RTSPawnReference && RTSSelectedActors.Num() > 0)
	{
		const FColor SelectionRim(255, 252, 245);
		for (const TWeakObjectPtr<AActor>& W : RTSSelectedActors)
		{
			if (AActor* A = W.Get())
			{
				// Pas de DrawDebugBox pour les personnages : ce serait toujours une AABB (boîte filaire) ;
				// la sélection visible repose sur Custom Depth sur le mesh (voir OnSelect sur NPC / objets).
				if (Cast<ACharacter>(A))
				{
					continue;
				}
				FVector Origin, Extent;
				ATheHouseObject::GetActorOutlineDebugBounds(A, Origin, Extent);
				if (Extent.GetMax() > KINDA_SMALL_NUMBER)
				{
					DrawDebugBox(GetWorld(), Origin, Extent, FQuat::Identity, SelectionRim, false, 0.f, 0, 2.5f);
				}
			}
		}
	}

    const bool bIsInRTSPlacementContext = (GetPawn() == RTSPawnReference);

    // Placement rotation : géré dans TheHouse_ApplyWheelZoom (molette interceptée par le viewport).

    if (bIsInRTSPlacementContext && PlacementState == EPlacementState::Previewing && StaffPlacementPreviewNpc)
    {
      UpdateStaffNpcPlacementPreview();
    }

    if (bIsInRTSPlacementContext && PlacementState == EPlacementState::Previewing && PreviewActor)
    {
      UpdatePlacementPreview();
    }

    // Fallback confirm (objets placés uniquement): indépendant des ActionMappings / EnhancedInput.
    // Le staff recruitment utilise déjà Input_SelectPressed / double-clic ; un second chemin ici
    // doublait ConfirmStaffNpcPlacement() (poll LMB + bind Select dans la même frame).

    if (!AnyRTSContextMenuOpen() && bIsInRTSPlacementContext && PlacementState == EPlacementState::Previewing && PreviewActor &&
        !bIsRotateModifierDown)
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
			PreviewActor->ClearPlacementBlockerActorCaches();
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
			PreviewActor->ClearPlacementBlockerActorCaches();
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
		PreviewActor->ClearPlacementBlockerActorCaches();
		PreviewActor->SetPlacementState(EObjectPlacementState::OverlapsWorld);
		if (TheHousePlacementDebug::IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Placement] Trace: no hit under cursor"));
		}
	}
}

void ATheHousePlayerController::UpdateStaffNpcPlacementPreview()
{
	if (!StaffPlacementPreviewNpc)
	{
		return;
	}

	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		bStaffNpcPlacementLocationValid = false;
		return;
	}

	FHitResult Hit;
	const FVector End = WorldOrigin + WorldDir * 100000.f;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(StaffNpcPlacementPreviewTrace), true);
	Params.AddIgnoredActor(this);
	if (RTSPawnReference)
	{
		Params.AddIgnoredActor(RTSPawnReference);
	}
	Params.AddIgnoredActor(StaffPlacementPreviewNpc);

	if (!GetWorld()->LineTraceSingleByChannel(Hit, WorldOrigin, End, ECC_Visibility, Params))
	{
		bStaffNpcPlacementLocationValid = false;
		return;
	}

	if (!ValidatePlacement(Hit))
	{
		bStaffNpcPlacementLocationValid = false;
		return;
	}

	FVector Loc = Hit.ImpactPoint;
	if (PlacementGrid.bEnableGridSnap && PlacementGrid.CellSize > KINDA_SMALL_NUMBER)
	{
		const FVector SnappedXY = SnapWorldToGridXY(Loc);
		Loc.X = SnappedXY.X;
		Loc.Y = SnappedXY.Y;
	}

	float HalfH = 96.f;
	if (UCapsuleComponent* Cap = StaffPlacementPreviewNpc->GetCapsuleComponent())
	{
		HalfH = Cap->GetScaledCapsuleHalfHeight();
	}
	Loc.Z = Hit.ImpactPoint.Z + HalfH;

	const FRotator Rot(0.f, PlacementPreviewYawDegrees, 0.f);
	StaffPlacementPreviewNpc->SetActorLocationAndRotation(Loc, Rot);
	bStaffNpcPlacementLocationValid = true;
}

bool ATheHousePlayerController::ValidatePlacement(const FHitResult& Hit) const
{
	const float Threshold = FMath::Clamp(PlacementGrid.MinUpNormalZ, 0.f, 1.f);
	return Hit.ImpactNormal.Z >= Threshold;
}

bool ATheHousePlayerController::HasRTSSelectionActorOtherThan(const AActor* Exclude) const
{
	for (const TWeakObjectPtr<AActor>& W : RTSSelectedActors)
	{
		if (AActor* A = W.Get())
		{
			if (Exclude && A == Exclude)
			{
				continue;
			}
			return true;
		}
	}
	return false;
}

namespace TheHouseStaffPlacementInternal
{
	static void ApplyStaffOfferRuntimeStatsIfRoster(ATheHouseNPCCharacter* Npc, const FTheHouseNPCStaffRosterOffer& Offer)
	{
		if (!Npc || !Offer.StaffArchetype)
		{
			return;
		}
		Npc->ApplyInitialHireFromRosterOffer(Offer);
	}
}

void ATheHousePlayerController::ConfigureStaffNpcDeferredFromOffer(ATheHouseNPCCharacter* Npc, const FTheHouseNPCStaffRosterOffer& Offer) const
{
	if (!Npc || !Offer.CharacterClass)
	{
		return;
	}

	if (Offer.StaffArchetype)
	{
		Npc->NPCArchetype = Offer.StaffArchetype;
	}
	else if (const ATheHouseNPCCharacter* CDO = Cast<ATheHouseNPCCharacter>(Offer.CharacterClass.GetDefaultObject()))
	{
		Npc->NPCArchetype = CDO->NPCArchetype;
	}

	if (Offer.MeshVariantRollIndex >= 0)
	{
		Npc->StaffMeshVariantRollIndex = Offer.MeshVariantRollIndex;
	}
	else
	{
		Npc->StaffMeshVariantRollIndex = INDEX_NONE;
	}
}

void ATheHousePlayerController::StaffUiSetRosterCategoryFilter(const FName CategoryId)
{
	StaffUiRosterBrowseCategoryId = CategoryId;
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::StaffUiClearRosterCategoryFilter()
{
	StaffUiRosterBrowseCategoryId = NAME_None;
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::RefreshNPCStaffRecruitmentOffers()
{
	NPCStaffRosterOffers.Reset();
	StaffUiRosterBrowseCategoryId = NAME_None;
	if (NPCStaffRecruitmentPool.Num() == 0)
	{
		RefreshRTSMainWidget();
		return;
	}

	int32 NextIndex = 0;
	for (const FTheHouseNPCStaffPoolSlotDef& Slot : NPCStaffRecruitmentPool)
	{
		if (!Slot.CharacterClass || !Slot.StaffArchetype)
		{
			continue;
		}

		const int32 Offers = FMath::Max(1, Slot.OffersPerRefresh);
		const float MulMin = FMath::Min(Slot.SalaryMultiplierMin, Slot.SalaryMultiplierMax);
		const float MulMax = FMath::Max(Slot.SalaryMultiplierMin, Slot.SalaryMultiplierMax);
		const int32 StarLo = FMath::Clamp(FMath::Min(Slot.StarRatingMin, Slot.StarRatingMax), 1, 5);
		const int32 StarHi = FMath::Clamp(FMath::Max(Slot.StarRatingMin, Slot.StarRatingMax), 1, 5);

		const float BaseSalary = FMath::Max(0.f, Slot.StaffArchetype->DefaultMonthlySalary);

		TSet<FString> UsedProceduralNamesThisSlot;
		auto PickProceduralDisplayName = [&]()
		{
			if (Slot.RandomGivenNames.Num() == 0 || Slot.RandomFamilyNames.Num() == 0)
			{
				return FText::GetEmpty();
			}
			for (int32 Attempt = 0; Attempt < 48; ++Attempt)
			{
				const int32 Gi = FMath::RandRange(0, Slot.RandomGivenNames.Num() - 1);
				const int32 Fi = FMath::RandRange(0, Slot.RandomFamilyNames.Num() - 1);
				const FText Composed = FText::Format(
					NSLOCTEXT("TheHouse", "StaffRosterProceduralFullName", "{0} {1}"),
					Slot.RandomGivenNames[Gi],
					Slot.RandomFamilyNames[Fi]);
				const FString Key = Composed.ToString();
				if (!UsedProceduralNamesThisSlot.Contains(Key))
				{
					UsedProceduralNamesThisSlot.Add(Key);
					return Composed;
				}
			}
			const int32 Gi = FMath::RandRange(0, Slot.RandomGivenNames.Num() - 1);
			const int32 Fi = FMath::RandRange(0, Slot.RandomFamilyNames.Num() - 1);
			return FText::Format(
				NSLOCTEXT("TheHouse", "StaffRosterProceduralFullName", "{0} {1}"),
				Slot.RandomGivenNames[Gi],
				Slot.RandomFamilyNames[Fi]);
		};

		for (int32 n = 0; n < Offers; ++n)
		{
			FTheHouseNPCStaffRosterOffer Offer;
			Offer.OfferIndexInRoster = NextIndex++;
			Offer.CharacterClass = Slot.CharacterClass;
			Offer.StaffArchetype = Slot.StaffArchetype;

			if (!Slot.StaffCategoryId.IsNone())
			{
				Offer.StaffCategoryId = Slot.StaffCategoryId;
			}
			else if (Slot.StaffArchetype && Slot.StaffArchetype->StaffRoleId != NAME_None)
			{
				Offer.StaffCategoryId = Slot.StaffArchetype->StaffRoleId;
			}
			else
			{
				Offer.StaffCategoryId = FName(TEXT("Staff"));
			}

			if (!Slot.StaffCategoryLabel.IsEmpty())
			{
				Offer.StaffCategoryLabel = Slot.StaffCategoryLabel;
			}
			else
			{
				Offer.StaffCategoryLabel = FText::FromName(Offer.StaffCategoryId);
			}

			const float Mul = FMath::FRandRange(MulMin, MulMax);
			Offer.MonthlySalary = FMath::Max(0.f, BaseSalary * Mul);
			Offer.StarRating = FMath::RandRange(StarLo, StarHi);
			Offer.Thumbnail = Slot.StaffArchetype ? Slot.StaffArchetype->StaffPortrait : nullptr;

			if (Slot.RandomDisplayNames.Num() > 0)
			{
				const int32 Ni = FMath::RandRange(0, Slot.RandomDisplayNames.Num() - 1);
				Offer.DisplayName = Slot.RandomDisplayNames[Ni];
			}
			else if (const FText ProcName = PickProceduralDisplayName(); !ProcName.IsEmpty())
			{
				Offer.DisplayName = ProcName;
			}
			else if (Slot.StaffArchetype && !Slot.StaffArchetype->DisplayName.IsEmpty())
			{
				// Identity > NPC|Archetype|Identity|DisplayName sur le Data Asset Staff.
				if (Offers > 1)
				{
					Offer.DisplayName = FText::Format(
						NSLOCTEXT("TheHouse", "StaffRosterArchetypeNameIndexed", "{0} ({1})"),
						Slot.StaffArchetype->DisplayName,
						FText::AsNumber(n + 1));
				}
				else
				{
					Offer.DisplayName = Slot.StaffArchetype->DisplayName;
				}
			}
			else
			{
				Offer.DisplayName = FText::Format(
					NSLOCTEXT("TheHouse", "StaffRosterGenericName", "{0} #{1}"),
					Offer.StaffCategoryLabel.IsEmpty() ? FText::FromName(Offer.StaffCategoryId) : Offer.StaffCategoryLabel,
					FText::AsNumber(Offer.OfferIndexInRoster));
			}

			const float Months = FMath::Max(0.f, Slot.HireCostSalaryMonths);
			Offer.HireCost = Months <= 0.f ? 0 : FMath::CeilToInt(Offer.MonthlySalary * Months);

			const int32 NumVar = Slot.StaffArchetype->SkeletalMeshVariants.Num();
			if (NumVar > 0)
			{
				Offer.MeshVariantRollIndex = FMath::RandRange(0, NumVar - 1);
			}
			else
			{
				Offer.MeshVariantRollIndex = INDEX_NONE;
			}

			NPCStaffRosterOffers.Add(Offer);
		}
	}

	RefreshRTSMainWidget();
}

void ATheHousePlayerController::ClearNPCStaffRecruitmentPoolSlots()
{
	NPCStaffRecruitmentPool.Reset();
	RefreshNPCStaffRecruitmentOffers();
}

void ATheHousePlayerController::AddNPCStaffRecruitmentPoolSlot(const FTheHouseNPCStaffPoolSlotDef& Slot, const bool bRebuildRosterAndUi)
{
	NPCStaffRecruitmentPool.Add(Slot);
	if (bRebuildRosterAndUi)
	{
		RefreshNPCStaffRecruitmentOffers();
	}
}

void ATheHousePlayerController::StartStaffNpcPlacementFromRosterOffer(int32 RosterOfferIndex)
{
	if (!NPCStaffRosterOffers.IsValidIndex(RosterOfferIndex))
	{
		return;
	}
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}

	const FTheHouseNPCStaffRosterOffer Pick = NPCStaffRosterOffers[RosterOfferIndex];
	if (Pick.HireCost > 0 && Money < Pick.HireCost)
	{
		return;
	}

	CancelPlacement();

	PendingStaffSpawnOffer = Pick;
	PendingStaffSpawnOfferRosterIndex = RosterOfferIndex;

	RTS_DeselectAll();

	UWorld* World = GetWorld();
	if (!World || !Pick.CharacterClass)
	{
		PendingStaffSpawnOffer = FTheHouseNPCStaffRosterOffer();
		PendingStaffSpawnOfferRosterIndex = INDEX_NONE;
		return;
	}

	AActor* Deferred = UGameplayStatics::BeginDeferredActorSpawnFromClass(
		World,
		Pick.CharacterClass,
		FTransform::Identity,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		this);

	ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(Deferred);
	if (!Npc)
	{
		PendingStaffSpawnOffer = FTheHouseNPCStaffRosterOffer();
		PendingStaffSpawnOfferRosterIndex = INDEX_NONE;
		return;
	}

	Npc->AutoPossessAI = EAutoPossessAI::Disabled;
	Npc->AIControllerClass = nullptr;

	Npc->bStaffPlacementPreviewActor = true;

	ConfigureStaffNpcDeferredFromOffer(Npc, PendingStaffSpawnOffer);

	UGameplayStatics::FinishSpawningActor(Npc, FTransform::Identity);

	StaffPlacementPreviewNpc = Npc;
	StaffPlacementPreviewNpc->SetActorHiddenInGame(false);
	StaffPlacementPreviewNpc->SetActorEnableCollision(false);
	StaffPlacementPreviewNpc->SetActorScale3D(FVector::OneVector);
	if (USkeletalMeshComponent* Sk = StaffPlacementPreviewNpc->GetMesh())
	{
		Sk->SetVisibility(true, /*bPropagateToChildren*/ true);
		Sk->SetHiddenInGame(false, /*bPropagateToChildren*/ true);
	}
	if (UCapsuleComponent* Cap = StaffPlacementPreviewNpc->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	TheHouseStaffPlacementInternal::ApplyStaffOfferRuntimeStatsIfRoster(StaffPlacementPreviewNpc, PendingStaffSpawnOffer);

	PlacementState = EPlacementState::Previewing;
	PlacementPreviewYawDegrees = 0.f;
	bStaffNpcPlacementLocationValid = false;

	UpdateStaffNpcPlacementPreview();
}

void ATheHousePlayerController::StartStaffNpcPlacementFromPalette(TSubclassOf<ATheHouseNPCCharacter> NPCClass, int32 HireCost)
{
	if (!NPCClass)
	{
		return;
	}
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}

	FTheHouseNPCStaffRosterOffer Offer;
	Offer.CharacterClass = NPCClass;
	Offer.HireCost = FMath::Max(0, HireCost);
	Offer.OfferIndexInRoster = INDEX_NONE;

	if (Offer.HireCost > 0 && Money < Offer.HireCost)
	{
		return;
	}

	CancelPlacement();

	PendingStaffSpawnOffer = Offer;
	PendingStaffSpawnOfferRosterIndex = INDEX_NONE;

	RTS_DeselectAll();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* Deferred = UGameplayStatics::BeginDeferredActorSpawnFromClass(
		World,
		PendingStaffSpawnOffer.CharacterClass,
		FTransform::Identity,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		this);

	ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(Deferred);
	if (!Npc)
	{
		PendingStaffSpawnOffer = FTheHouseNPCStaffRosterOffer();
		PendingStaffSpawnOfferRosterIndex = INDEX_NONE;
		return;
	}

	Npc->AutoPossessAI = EAutoPossessAI::Disabled;
	Npc->AIControllerClass = nullptr;

	Npc->bStaffPlacementPreviewActor = true;

	ConfigureStaffNpcDeferredFromOffer(Npc, PendingStaffSpawnOffer);

	UGameplayStatics::FinishSpawningActor(Npc, FTransform::Identity);

	StaffPlacementPreviewNpc = Npc;
	StaffPlacementPreviewNpc->SetActorHiddenInGame(false);
	StaffPlacementPreviewNpc->SetActorEnableCollision(false);
	StaffPlacementPreviewNpc->SetActorScale3D(FVector::OneVector);
	if (USkeletalMeshComponent* Sk = StaffPlacementPreviewNpc->GetMesh())
	{
		Sk->SetVisibility(true, /*bPropagateToChildren*/ true);
		Sk->SetHiddenInGame(false, /*bPropagateToChildren*/ true);
	}
	if (UCapsuleComponent* Cap = StaffPlacementPreviewNpc->GetCapsuleComponent())
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	TheHouseStaffPlacementInternal::ApplyStaffOfferRuntimeStatsIfRoster(StaffPlacementPreviewNpc, PendingStaffSpawnOffer);

	PlacementState = EPlacementState::Previewing;
	PlacementPreviewYawDegrees = 0.f;
	bStaffNpcPlacementLocationValid = false;

	UpdateStaffNpcPlacementPreview();
}

void ATheHousePlayerController::ConfirmStaffNpcPlacement()
{
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}
	if (PlacementState != EPlacementState::Previewing || !StaffPlacementPreviewNpc || !bStaffNpcPlacementLocationValid)
	{
		return;
	}
	if (bStaffNpcConfirmInFlight)
	{
		return;
	}
	bStaffNpcConfirmInFlight = true;

	const FTheHouseNPCStaffRosterOffer OfferCopy = PendingStaffSpawnOffer;
	const int32 OfferSourceRosterIndex = PendingStaffSpawnOfferRosterIndex;

	struct FResetStaffConfirm
	{
		bool& bFlag;
		~FResetStaffConfirm() { bFlag = false; }
	} ResetStaffConfirm{ bStaffNpcConfirmInFlight };

	if (OfferCopy.HireCost > Money)
	{
		return;
	}

	const FTransform SpawnXform = StaffPlacementPreviewNpc->GetActorTransform();

	CancelPlacement();

	if (!OfferCopy.CharacterClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AActor* DeferredLive = UGameplayStatics::BeginDeferredActorSpawnFromClass(
		World,
		OfferCopy.CharacterClass,
		SpawnXform,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		this);

	ATheHouseNPCCharacter* Live = Cast<ATheHouseNPCCharacter>(DeferredLive);
	if (!Live)
	{
		return;
	}

	ConfigureStaffNpcDeferredFromOffer(Live, OfferCopy);

	UGameplayStatics::FinishSpawningActor(Live, SpawnXform);

	TheHouseStaffPlacementInternal::ApplyStaffOfferRuntimeStatsIfRoster(Live, OfferCopy);

	// Mark as employed for payroll (start counting from now).
	Live->MarkStaffEmployedAtInGameTime(InGameTimeSeconds, InGameDayIndex);

	if (OfferCopy.HireCost > 0)
	{
		Money = FMath::Max(0, Money - OfferCopy.HireCost);
	}

	// Consommer l’offre du roster (si elle venait du roster) : elle ne doit plus être embauchable telle quelle.
	bool bConsumedFromRoster = false;
	if (OfferSourceRosterIndex >= 0 && NPCStaffRosterOffers.Num() > 0)
	{
		int32 RemoveIdx = INDEX_NONE;
		// 1) Si l’index d’origine correspond encore à la même offre, retirer directement.
		if (NPCStaffRosterOffers.IsValidIndex(OfferSourceRosterIndex))
		{
			const FTheHouseNPCStaffRosterOffer& Current = NPCStaffRosterOffers[OfferSourceRosterIndex];
			if (Current.OfferIndexInRoster == OfferCopy.OfferIndexInRoster && Current.CharacterClass == OfferCopy.CharacterClass)
			{
				RemoveIdx = OfferSourceRosterIndex;
			}
		}
		// 2) Sinon chercher par identifiant/clé stable.
		if (RemoveIdx == INDEX_NONE)
		{
			for (int32 i = 0; i < NPCStaffRosterOffers.Num(); ++i)
			{
				const FTheHouseNPCStaffRosterOffer& O = NPCStaffRosterOffers[i];
				if (O.OfferIndexInRoster == OfferCopy.OfferIndexInRoster && O.CharacterClass == OfferCopy.CharacterClass)
				{
					RemoveIdx = i;
					break;
				}
			}
		}
		if (RemoveIdx != INDEX_NONE)
		{
			NPCStaffRosterOffers.RemoveAt(RemoveIdx);
			bConsumedFromRoster = true;
		}
	}

	// Si on vient de consommer la dernière offre, régénérer pour éviter une liste vide.
	if (bConsumedFromRoster && NPCStaffRosterOffers.Num() == 0 && NPCStaffRecruitmentPool.Num() > 0)
	{
		RefreshNPCStaffRecruitmentOffers();
	}
	else
	{
		RefreshRTSMainWidget();
	}
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

	const bool bChargeCatalogPurchase =
		!bRelocationPreviewActive && PendingStockConsumeIndex == INDEX_NONE;
	int32 CatalogPurchasePrice = 0;
	if (bChargeCatalogPurchase)
	{
		CatalogPurchasePrice = GetPurchasePriceForClass(PreviewActor->GetClass());
		if (Money < CatalogPurchasePrice)
		{
			if (TheHousePlacementDebug::IsEnabled())
			{
				UE_LOG(LogTemp, Warning, TEXT("[Placement] Confirm blocked: argent insuffisant (besoin %d, a %d)"),
					CatalogPurchasePrice, Money);
			}
			return;
		}
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

	if (bChargeCatalogPurchase && CatalogPurchasePrice > 0)
	{
		Money = FMath::Max(0, Money - CatalogPurchasePrice);
	}

	if (PendingStockConsumeIndex != INDEX_NONE && StoredPlaceableStacks.IsValidIndex(PendingStockConsumeIndex))
	{
		FTheHouseStoredStack& Stack = StoredPlaceableStacks[PendingStockConsumeIndex];
		if (PreviewActor && PreviewActor->GetClass() == Stack.ObjectClass)
		{
			Stack.Count = FMath::Max(0, Stack.Count - 1);
			if (Stack.Count <= 0)
			{
				StoredPlaceableStacks.RemoveAt(PendingStockConsumeIndex);
			}
		}
	}
	PendingStockConsumeIndex = INDEX_NONE;

	bRelocationPreviewActive = false;

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

void ATheHousePlayerController::EnsureDefaultNPCRTSContextMenuDefs()
{
	NPCRTSContextMenuOptionDefs.RemoveAll([](const FTheHouseRTSContextMenuOptionDef& D)
	{
		return D.OptionId.IsNone();
	});

	if (NPCRTSContextMenuOptionDefs.Num() == 0)
	{
		using namespace TheHouseNPCContextIds;

		FTheHouseRTSContextMenuOptionDef DInsp;
		DInsp.OptionId = InspectNPC;
		DInsp.Label = NSLOCTEXT("TheHouse", "RTS_NPCMenu_Inspect", "Inspecter");
		NPCRTSContextMenuOptionDefs.Add(DInsp);

		FTheHouseRTSContextMenuOptionDef DUse;
		DUse.OptionId = UsePlacedObjectAtCursor;
		DUse.Label = NSLOCTEXT("TheHouse", "RTS_NPCMenu_UseObject", "Utiliser l’objet (curseur)");
		NPCRTSContextMenuOptionDefs.Add(DUse);
	}
}

void ATheHousePlayerController::EnsureDefaultRTSContextMenuDefs()
{
	RTSContextMenuOptionDefs.RemoveAll([](const FTheHouseRTSContextMenuOptionDef& D) {
		return D.OptionId.IsNone();
	});

	if (RTSContextMenuOptionDefs.Num() == 0)
	{
		FTheHouseRTSContextMenuOptionDef DDel;
		DDel.OptionId = TheHouseRTSContextIds::DeleteObject;
		DDel.Label = NSLOCTEXT("TheHouse", "RTS_RemoveObject", "Vendre");
		RTSContextMenuOptionDefs.Add(DDel);

		FTheHouseRTSContextMenuOptionDef DSt;
		DSt.OptionId = TheHouseRTSContextIds::StoreObject;
		DSt.Label = NSLOCTEXT("TheHouse", "RTS_StoreObject", "Mettre en stock");
		RTSContextMenuOptionDefs.Add(DSt);
	}

	auto HasOptionId = [this](FName Id) -> bool {
		for (const FTheHouseRTSContextMenuOptionDef& D : RTSContextMenuOptionDefs)
		{
			if (D.OptionId == Id)
			{
				return true;
			}
		}
		return false;
	};
	using namespace TheHouseRTSContextIds;
	if (!HasOptionId(RelocateObject))
	{
		FTheHouseRTSContextMenuOptionDef D;
		D.OptionId = RelocateObject;
		D.Label = NSLOCTEXT("TheHouse", "RTS_RelocateObject", "Déplacer");
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
		// Slate C++ pur (RebuildWidget) : affichage fiable sans Widget Blueprint / OptionsBox.
		RTSContextMenuWidgetClass = UTheHouseRTSContextMenuWidget::StaticClass();
	}
	if (!NPCRTSContextMenuWidgetClass)
	{
		NPCRTSContextMenuWidgetClass = UTheHouseNPCRTSContextMenuUMGWidget::StaticClass();
	}
	if (!NPCOrderContextMenuWidgetClass)
	{
		NPCOrderContextMenuWidgetClass = UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
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

	if (RTSNPCContextMenuInstance)
	{
		RTSNPCContextMenuInstance->RemoveFromParent();
		RTSNPCContextMenuInstance = nullptr;
	}

	if (RTSNPCOrderOnObjectMenuInstance)
	{
		RTSNPCOrderOnObjectMenuInstance->RemoveFromParent();
		RTSNPCOrderOnObjectMenuInstance = nullptr;
	}

	if (RTSNPCOrderOnNPCMenuInstance)
	{
		RTSNPCOrderOnNPCMenuInstance->RemoveFromParent();
		RTSNPCOrderOnNPCMenuInstance = nullptr;
	}

	if (RTSNPCOrderOnGroundMenuInstance)
	{
		RTSNPCOrderOnGroundMenuInstance->RemoveFromParent();
		RTSNPCOrderOnGroundMenuInstance = nullptr;
	}

	// Restore RTS input mode so clicks go back to the game when menu closes.
	if (Cast<ATheHouseCameraPawn>(GetPawn()) == RTSPawnReference && !bIsRotateModifierDown)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		TheHouse_FocusKeyboardOnGameViewport();
	}
}

bool ATheHousePlayerController::IsRtsCameraRotateModifierPhysicallyDown() const
{
	return TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::LeftAlt) ||
		   TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::RightAlt);
}

void ATheHousePlayerController::CollectMergedRTSLineHitsUnderCursor(TMap<AActor*, FHitResult>& OutBestHitPerActor) const
{
	OutBestHitPerActor.Reset();

	auto ConsiderHit = [&OutBestHitPerActor](const FHitResult& H)
	{
		AActor* A = H.GetActor();
		if (!IsValid(A))
		{
			return;
		}
		if (FHitResult* Existing = OutBestHitPerActor.Find(A))
		{
			if (H.Distance < Existing->Distance)
			{
				*Existing = H;
			}
		}
		else
		{
			OutBestHitPerActor.Add(A, H);
		}
	};

	FHitResult CursorHit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, CursorHit))
	{
		ConsiderHit(CursorHit);
	}

	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
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

	TArray<FHitResult> Hits;
	auto RunChannelTrace = [&](ECollisionChannel Channel)
	{
		Hits.Reset();
		if (World->LineTraceMultiByChannel(Hits, WorldOrigin, End, Channel, Params))
		{
			for (const FHitResult& H : Hits)
			{
				ConsiderHit(H);
			}
		}
	};

	RunChannelTrace(ECC_Visibility);
	RunChannelTrace(ECC_WorldDynamic);
	RunChannelTrace(ECC_WorldStatic);

	Hits.Reset();
	FCollisionObjectQueryParams ObjParams;
	ObjParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	if (World->LineTraceMultiByObjectType(Hits, WorldOrigin, End, ObjParams, Params))
	{
		for (const FHitResult& H : Hits)
		{
			ConsiderHit(H);
		}
	}
}

bool ATheHousePlayerController::TryResolveGroundClickLocation(const TMap<AActor*, FHitResult>& MergedHits, FVector& OutLocation) const
{
	float BestDist = TNumericLimits<float>::Max();
	bool bFound = false;

	for (const TPair<AActor*, FHitResult>& P : MergedHits)
	{
		const FHitResult& H = P.Value;
		// Surface « au sol » : évite murs / murs invisibles ; ajustez le seuil si besoin.
		if (H.Normal.Z >= 0.62f && H.Distance < BestDist)
		{
			BestDist = H.Distance;
			OutLocation = H.Location;
			bFound = true;
		}
	}

	if (bFound)
	{
		return true;
	}

	FVector WorldOrigin, WorldDir;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDir))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector TraceEnd = WorldOrigin + WorldDir * 200000.f;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TheHouseRTSGroundPick), false);
	if (RTSPawnReference)
	{
		Params.AddIgnoredActor(RTSPawnReference);
	}
	if (PreviewActor)
	{
		Params.AddIgnoredActor(PreviewActor);
	}

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, WorldOrigin, TraceEnd, ECC_WorldStatic, Params))
	{
		OutLocation = Hit.Location;
		return true;
	}

	return false;
}

AActor* ATheHousePlayerController::FindPrimarySelectableActorUnderCursorRTS() const
{
	TMap<AActor*, FHitResult> Merged;
	CollectMergedRTSLineHitsUnderCursor(Merged);

	AActor* BestActor = nullptr;
	float BestDist = TNumericLimits<float>::Max();

	for (const TPair<AActor*, FHitResult>& P : Merged)
	{
		AActor* Act = P.Key;
		if (!IsValid(Act) || !Act->Implements<UTheHouseSelectable>())
		{
			continue;
		}

		ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Act);
		if (!Sel || !Sel->IsSelectable())
		{
			continue;
		}

		if (ATheHouseObject* Obj = Cast<ATheHouseObject>(Act))
		{
			if (!IsValid(Obj) || Obj == PreviewActor || Obj->IsPlacementPreviewActor())
			{
				continue;
			}
		}

		const float D = P.Value.Distance;
		const bool bCloser = D < BestDist;
		const bool bTiePreferNpc =
			FMath::IsNearlyEqual(D, BestDist, KINDA_SMALL_NUMBER) && Cast<ATheHouseNPCCharacter>(Act) &&
			!Cast<ATheHouseNPCCharacter>(BestActor);

		if (bCloser || bTiePreferNpc)
		{
			BestDist = D;
			BestActor = Act;
		}
	}

	return BestActor;
}

void ATheHousePlayerController::AppendSelectableNPCsIntersectingRTSBox(
	const FVector2D& RectCornerA,
	const FVector2D& RectCornerB,
	TArray<AActor*>& InOutActors) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector2D RectMin(FMath::Min(RectCornerA.X, RectCornerB.X), FMath::Min(RectCornerA.Y, RectCornerB.Y));
	const FVector2D RectMax(FMath::Max(RectCornerA.X, RectCornerB.X), FMath::Max(RectCornerA.Y, RectCornerB.Y));

	for (TActorIterator<ATheHouseNPCCharacter> It(World); It; ++It)
	{
		ATheHouseNPCCharacter* Npc = *It;
		if (!IsValid(Npc) || !Npc->IsSelectable())
		{
			continue;
		}

		const USkeletalMeshComponent* Mesh = Npc->GetMesh();
		if (!Mesh)
		{
			continue;
		}

		const FBoxSphereBounds& MB = Mesh->Bounds;
		const FVector O = MB.Origin;
		const FVector E = MB.BoxExtent;

		constexpr float Huge = TNumericLimits<float>::Max();
		FVector2D ScreenMin(Huge, Huge);
		FVector2D ScreenMax(-Huge, -Huge);
		int32 ProjectedCount = 0;

		for (int32 sx = -1; sx <= 1; sx += 2)
		{
			for (int32 sy = -1; sy <= 1; sy += 2)
			{
				for (int32 sz = -1; sz <= 1; sz += 2)
				{
					const FVector Corner(O.X + static_cast<float>(sx) * E.X, O.Y + static_cast<float>(sy) * E.Y, O.Z + static_cast<float>(sz) * E.Z);
					FVector2D Scr;
					if (!ProjectWorldLocationToScreen(Corner, Scr, true))
					{
						continue;
					}
					++ProjectedCount;
					ScreenMin.X = FMath::Min(ScreenMin.X, Scr.X);
					ScreenMin.Y = FMath::Min(ScreenMin.Y, Scr.Y);
					ScreenMax.X = FMath::Max(ScreenMax.X, Scr.X);
					ScreenMax.Y = FMath::Max(ScreenMax.Y, Scr.Y);
				}
			}
		}

		if (ProjectedCount == 0)
		{
			continue;
		}

		if (ScreenMax.X < RectMin.X || RectMax.X < ScreenMin.X || ScreenMax.Y < RectMin.Y || RectMax.Y < ScreenMin.Y)
		{
			continue;
		}

		InOutActors.AddUnique(Npc);
	}
}

ATheHouseObject* ATheHousePlayerController::FindPlacedTheHouseObjectUnderCursor() const
{
	TMap<AActor*, FHitResult> Merged;
	CollectMergedRTSLineHitsUnderCursor(Merged);

	ATheHouseObject* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();

	for (const TPair<AActor*, FHitResult>& P : Merged)
	{
		ATheHouseObject* Obj = Cast<ATheHouseObject>(P.Key);
		if (!IsValid(Obj) || Obj == PreviewActor || Obj->IsPlacementPreviewActor())
		{
			continue;
		}
		const float D = P.Value.Distance;
		if (D < BestDist)
		{
			BestDist = D;
			Best = Obj;
		}
	}

	return Best;
}

FVector2D ATheHousePlayerController::TheHouse_GetContextMenuViewportPosition() const
{
	FVector2D MenuScreenPos(0.f, 0.f);
	if (FSlateApplication::IsInitialized() && GetWorld())
	{
		MenuScreenPos = UWidgetLayoutLibrary::GetMousePositionOnViewport(GetWorld());
	}
	else
	{
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (GetMousePosition(MouseX, MouseY))
		{
			MenuScreenPos = FVector2D(MouseX, MouseY);
		}
		else
		{
			int32 SizeX = 0;
			int32 SizeY = 0;
			GetViewportSize(SizeX, SizeY);
			if (SizeX > 0 && SizeY > 0)
			{
				MenuScreenPos = FVector2D(SizeX * 0.5f, SizeY * 0.5f);
			}
		}
	}
	return MenuScreenPos;
}

void ATheHousePlayerController::TheHouse_OpenRTSContextMenuShared(ATheHouseObject* Obj, const FVector2D& MenuScreenPos)
{
	if (!IsValid(Obj))
	{
		return;
	}

	EnsureDefaultRTSContextMenuDefs();

	if (UWorld* WorldForTime = GetWorld())
	{
		const double NowSec = WorldForTime->GetTimeSeconds();
		if (LastRTSContextMenuTryOpenTimeSeconds >= 0.0 &&
			(NowSec - LastRTSContextMenuTryOpenTimeSeconds) < TheHouse_RTSContextMenuTryOpenMinIntervalSeconds)
		{
			return;
		}
	}

	CloseRTSContextMenu();
	RTS_SelectExclusive(Obj);
	ClosePlacedObjectSettingsWidget();

	UClass* MenuWidgetClass = RTSContextMenuWidgetClass
		? RTSContextMenuWidgetClass.Get()
		: UTheHouseRTSContextMenuWidget::StaticClass();
	{
		UClass* UMGMenuType = UTheHouseRTSContextMenuUMGWidget::StaticClass();
		UClass* SlateMenuType = UTheHouseRTSContextMenuWidget::StaticClass();
		if (!MenuWidgetClass->IsChildOf(UMGMenuType) && !MenuWidgetClass->IsChildOf(SlateMenuType))
		{
#if !UE_BUILD_SHIPPING
			UE_LOG(LogTemp, Error,
				TEXT("[RTSContext] RTSContextMenuWidgetClass (%s) doit hériter de UTheHouseRTSContextMenuUMGWidget ou UTheHouseRTSContextMenuWidget. "
					"Corrigez le Class Defaults du Player Controller. Utilisation du menu Slate C++ par défaut."),
				*GetNameSafe(MenuWidgetClass));
#endif
			MenuWidgetClass = SlateMenuType;
		}
	}

	RTSContextMenuInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (!RTSContextMenuInstance)
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTSContext] CreateWidget failed"), FColor::Red);
		return;
	}

	RTSContextMenuInstance->SetVisibility(ESlateVisibility::Visible);
	RTSContextMenuInstance->SetRenderOpacity(1.f);
	RTSContextMenuInstance->SetIsEnabled(true);
	RTSContextMenuInstance->AddToViewport(100000);

	bool bOpenedMenuContent = false;
	if (UTheHouseRTSContextMenuUMGWidget* UMG = Cast<UTheHouseRTSContextMenuUMGWidget>(RTSContextMenuInstance))
	{
		UMG->OpenForTarget(this, Obj, MenuScreenPos);
		bOpenedMenuContent = true;
	}
	else if (UTheHouseRTSContextMenuWidget* SlateMenu = Cast<UTheHouseRTSContextMenuWidget>(RTSContextMenuInstance))
	{
		SlateMenu->OpenForTarget(this, Obj, MenuScreenPos);
		bOpenedMenuContent = true;
	}

	if (!bOpenedMenuContent && RTSContextMenuInstance)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("[RTSContext] OpenForTarget non appelé (cast widget impossible). Classe=%s"), *GetNameSafe(RTSContextMenuInstance->GetClass()));
#endif
		RTSContextMenuInstance->RemoveFromParent();
		RTSContextMenuInstance = nullptr;
		return;
	}

	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		if (RTSContextMenuInstance)
		{
			InputMode.SetWidgetToFocus(RTSContextMenuInstance->TakeWidget());
		}
		SetInputMode(InputMode);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
	}

	TheHouseRTSContextDebug::Screen(
		this,
		FString::Printf(TEXT("[RTSContext] Opened (%s) for %s at (%.0f,%.0f) | InViewport=%s"),
			*GetNameSafe(RTSContextMenuInstance->GetClass()),
			*Obj->GetName(),
			MenuScreenPos.X, MenuScreenPos.Y,
			RTSContextMenuInstance->IsInViewport() ? TEXT("yes") : TEXT("no")),
		FColor::Green);

	if (UWorld* WorldForTime = GetWorld())
	{
		LastRTSContextMenuTryOpenTimeSeconds = WorldForTime->GetTimeSeconds();
	}
}

void ATheHousePlayerController::IssueMoveOrdersToSelectedNPCsAtLocation(const FVector& GoalLocation)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FVector TargetLocation = GoalLocation;
	const bool bOnNav = TheHouseNPCOrdersInternal::ProjectPointToNavOrKeep(World, GoalLocation, TargetLocation);
#if !UE_BUILD_SHIPPING
	if (!bOnNav)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RTS|MoveTo] Point hors NavMesh (projection impossible). Ajoutez/étendez Nav Mesh Bounds Volume, touche P en jeu. Point=%s"),
			*GoalLocation.ToString());
	}
#endif

	for (const TWeakObjectPtr<AActor>& Weak : RTSSelectedActors)
	{
		ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(Weak.Get());
		if (!IsValid(Npc) || !Npc->IsSelectable())
		{
			continue;
		}

		AAIController* AI = Cast<AAIController>(Npc->GetController());
		if (!AI)
		{
#if !UE_BUILD_SHIPPING
			UE_LOG(LogTemp, Warning,
				TEXT("[RTS|MoveTo] Aucun AIController sur %s (vérifiez Auto Possess AI et AI Controller Class sur le Blueprint PNJ)."),
				*Npc->GetName());
#endif
			continue;
		}

		if (ATheHouseNPCAIController* HouseAI = Cast<ATheHouseNPCAIController>(AI))
		{
			HouseAI->ClearScriptedAttackOrder();
		}

		FAIMoveRequest Req;
		Req.SetGoalLocation(TargetLocation);
		Req.SetAcceptanceRadius(52.f);
		Req.SetUsePathfinding(true);
		Req.SetAllowPartialPath(true);

		const EPathFollowingRequestResult::Type MoveResult = AI->MoveTo(Req);
#if !UE_BUILD_SHIPPING
		if (MoveResult == EPathFollowingRequestResult::Failed)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RTS|MoveTo] MoveTo a échoué pour %s (NavMesh, blocage, agent…). Goal=%s"),
				*Npc->GetName(),
				*TargetLocation.ToString());
		}
#endif
	}
}

void ATheHousePlayerController::TheHouse_OpenRTSContextMenuForNPCShared(ATheHouseNPCCharacter* Npc, const FVector2D& MenuScreenPos)
{
	if (!IsValid(Npc))
	{
		return;
	}

	EnsureDefaultNPCRTSContextMenuDefs();

	if (UWorld* WorldForTime = GetWorld())
	{
		const double NowSec = WorldForTime->GetTimeSeconds();
		if (LastRTSContextMenuTryOpenTimeSeconds >= 0.0 &&
			(NowSec - LastRTSContextMenuTryOpenTimeSeconds) < TheHouse_RTSContextMenuTryOpenMinIntervalSeconds)
		{
			return;
		}
	}

	CloseRTSContextMenu();
	ClosePlacedObjectSettingsWidget();

	UClass* MenuWidgetClass = NPCRTSContextMenuWidgetClass ? NPCRTSContextMenuWidgetClass.Get()
														   : UTheHouseNPCRTSContextMenuUMGWidget::StaticClass();

	if (!MenuWidgetClass->IsChildOf(UTheHouseNPCRTSContextMenuUMGWidget::StaticClass()))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error,
			TEXT("[RTSNPCContext] NPCRTSContextMenuWidgetClass (%s) doit hériter de UTheHouseNPCRTSContextMenuUMGWidget — fallback classe C++."),
			*GetNameSafe(MenuWidgetClass));
#endif
		MenuWidgetClass = UTheHouseNPCRTSContextMenuUMGWidget::StaticClass();
	}

	RTSNPCContextMenuInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (!RTSNPCContextMenuInstance)
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTSNPCContext] CreateWidget failed"), FColor::Red);
		return;
	}

	RTSNPCContextMenuInstance->SetVisibility(ESlateVisibility::Visible);
	RTSNPCContextMenuInstance->SetRenderOpacity(1.f);
	RTSNPCContextMenuInstance->SetIsEnabled(true);
	RTSNPCContextMenuInstance->AddToViewport(100000);

	bool bOpenedMenuContent = false;
	if (UTheHouseNPCRTSContextMenuUMGWidget* UMG = Cast<UTheHouseNPCRTSContextMenuUMGWidget>(RTSNPCContextMenuInstance))
	{
		UMG->OpenForNPC(this, Npc, MenuScreenPos);
		bOpenedMenuContent = true;
	}

	if (!bOpenedMenuContent && RTSNPCContextMenuInstance)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("[RTSNPCContext] OpenForNPC impossible (cast). Classe=%s"), *GetNameSafe(RTSNPCContextMenuInstance->GetClass()));
#endif
		RTSNPCContextMenuInstance->RemoveFromParent();
		RTSNPCContextMenuInstance = nullptr;
		return;
	}

	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		if (RTSNPCContextMenuInstance)
		{
			InputMode.SetWidgetToFocus(RTSNPCContextMenuInstance->TakeWidget());
		}
		SetInputMode(InputMode);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
	}

	TheHouseRTSContextDebug::Screen(
		this,
		FString::Printf(TEXT("[RTSNPCContext] Opened (%s) for %s at (%.0f,%.0f)"),
			*GetNameSafe(RTSNPCContextMenuInstance->GetClass()),
			*Npc->GetName(),
			MenuScreenPos.X, MenuScreenPos.Y),
		FColor::Green);

	if (UWorld* WorldForTime = GetWorld())
	{
		LastRTSContextMenuTryOpenTimeSeconds = WorldForTime->GetTimeSeconds();
	}
}

bool ATheHousePlayerController::IsActorInRTSSelection(AActor* Actor) const
{
	if (!IsValid(Actor))
	{
		return false;
	}
	for (const TWeakObjectPtr<AActor>& W : RTSSelectedActors)
	{
		if (W.Get() == Actor)
		{
			return true;
		}
	}
	return false;
}

bool ATheHousePlayerController::HasSelectedNPCsForRTSOrders() const
{
	for (const TWeakObjectPtr<AActor>& W : RTSSelectedActors)
	{
		if (ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(W.Get()))
		{
			if (IsValid(Npc) && Npc->IsSelectable())
			{
				return true;
			}
		}
	}
	return false;
}

void ATheHousePlayerController::GetSelectedNPCsForRTSOrders(TArray<ATheHouseNPCCharacter*>& OutNPCs) const
{
	OutNPCs.Reset();
	for (const TWeakObjectPtr<AActor>& W : RTSSelectedActors)
	{
		if (ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(W.Get()))
		{
			if (IsValid(Npc) && Npc->IsSelectable())
			{
				OutNPCs.Add(Npc);
			}
		}
	}
}

void ATheHousePlayerController::TheHouse_OpenNPCOrderOnObjectMenuShared(ATheHouseObject* Obj, const FVector2D& MenuScreenPos)
{
	if (!IsValid(Obj) || Obj == PreviewActor || Obj->IsPlacementPreviewActor())
	{
		return;
	}

	if (UWorld* WorldForTime = GetWorld())
	{
		const double NowSec = WorldForTime->GetTimeSeconds();
		if (LastRTSContextMenuTryOpenTimeSeconds >= 0.0 &&
			(NowSec - LastRTSContextMenuTryOpenTimeSeconds) < TheHouse_RTSContextMenuTryOpenMinIntervalSeconds)
		{
			return;
		}
	}

	CloseRTSContextMenu();
	ClosePlacedObjectSettingsWidget();

	NPCOrderOnObjectRuntimeOptionDefs.Reset();
	GatherNPCOrderActionsOnObject(Obj, NPCOrderOnObjectRuntimeOptionDefs);

	UClass* MenuWidgetClass = NPCOrderContextMenuWidgetClass ? NPCOrderContextMenuWidgetClass.Get()
															 : UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
	if (!MenuWidgetClass->IsChildOf(UTheHouseNPCOrderContextMenuUMGWidget::StaticClass()))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error,
			TEXT("[RTS|NPCOrder] NPCOrderContextMenuWidgetClass (%s) doit hériter de UTheHouseNPCOrderContextMenuUMGWidget — fallback C++."),
			*GetNameSafe(MenuWidgetClass));
#endif
		MenuWidgetClass = UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
	}

	RTSNPCOrderOnObjectMenuInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (!RTSNPCOrderOnObjectMenuInstance)
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTS|NPCOrder|Obj] CreateWidget failed"), FColor::Red);
		return;
	}

	RTSNPCOrderOnObjectMenuInstance->SetVisibility(ESlateVisibility::Visible);
	RTSNPCOrderOnObjectMenuInstance->SetRenderOpacity(1.f);
	RTSNPCOrderOnObjectMenuInstance->SetIsEnabled(true);
	RTSNPCOrderOnObjectMenuInstance->AddToViewport(100000);

	bool bOpenedMenuContent = false;
	if (UTheHouseNPCOrderContextMenuUMGWidget* UMG = Cast<UTheHouseNPCOrderContextMenuUMGWidget>(RTSNPCOrderOnObjectMenuInstance))
	{
		UMG->OpenForOrderOnObject(this, Obj, MenuScreenPos);
		bOpenedMenuContent = true;
	}

	if (!bOpenedMenuContent && RTSNPCOrderOnObjectMenuInstance)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("[RTS|NPCOrder|Obj] OpenForOrderOnObject impossible (cast). Classe=%s"),
			*GetNameSafe(RTSNPCOrderOnObjectMenuInstance->GetClass()));
#endif
		RTSNPCOrderOnObjectMenuInstance->RemoveFromParent();
		RTSNPCOrderOnObjectMenuInstance = nullptr;
		return;
	}

	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		if (RTSNPCOrderOnObjectMenuInstance)
		{
			InputMode.SetWidgetToFocus(RTSNPCOrderOnObjectMenuInstance->TakeWidget());
		}
		SetInputMode(InputMode);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
	}

#if !UE_BUILD_SHIPPING
	TheHouseRTSContextDebug::Screen(
		this,
		FString::Printf(TEXT("[RTS|NPCOrder|Obj] Opened (%s) for %s"),
			*GetNameSafe(RTSNPCOrderOnObjectMenuInstance->GetClass()),
			*Obj->GetName()),
		FColor::Green);
#endif

	if (UWorld* WorldForTime = GetWorld())
	{
		LastRTSContextMenuTryOpenTimeSeconds = WorldForTime->GetTimeSeconds();
	}
}

void ATheHousePlayerController::TheHouse_OpenNPCOrderOnNPCMenuShared(ATheHouseNPCCharacter* Npc, const FVector2D& MenuScreenPos)
{
	if (!IsValid(Npc) || !Npc->IsSelectable())
	{
		return;
	}

	if (UWorld* WorldForTime = GetWorld())
	{
		const double NowSec = WorldForTime->GetTimeSeconds();
		if (LastRTSContextMenuTryOpenTimeSeconds >= 0.0 &&
			(NowSec - LastRTSContextMenuTryOpenTimeSeconds) < TheHouse_RTSContextMenuTryOpenMinIntervalSeconds)
		{
			return;
		}
	}

	CloseRTSContextMenu();
	ClosePlacedObjectSettingsWidget();

	NPCOrderOnNPCRuntimeOptionDefs.Reset();
	GatherNPCOrderActionsOnNPC(Npc, NPCOrderOnNPCRuntimeOptionDefs);

	UClass* MenuWidgetClass = NPCOrderContextMenuWidgetClass ? NPCOrderContextMenuWidgetClass.Get()
															 : UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
	if (!MenuWidgetClass->IsChildOf(UTheHouseNPCOrderContextMenuUMGWidget::StaticClass()))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error,
			TEXT("[RTS|NPCOrder|NPC] NPCOrderContextMenuWidgetClass (%s) doit hériter de UTheHouseNPCOrderContextMenuUMGWidget — fallback C++."),
			*GetNameSafe(MenuWidgetClass));
#endif
		MenuWidgetClass = UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
	}

	RTSNPCOrderOnNPCMenuInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (!RTSNPCOrderOnNPCMenuInstance)
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTS|NPCOrder|NPC] CreateWidget failed"), FColor::Red);
		return;
	}

	RTSNPCOrderOnNPCMenuInstance->SetVisibility(ESlateVisibility::Visible);
	RTSNPCOrderOnNPCMenuInstance->SetRenderOpacity(1.f);
	RTSNPCOrderOnNPCMenuInstance->SetIsEnabled(true);
	RTSNPCOrderOnNPCMenuInstance->AddToViewport(100000);

	bool bOpenedMenuContent = false;
	if (UTheHouseNPCOrderContextMenuUMGWidget* UMG = Cast<UTheHouseNPCOrderContextMenuUMGWidget>(RTSNPCOrderOnNPCMenuInstance))
	{
		UMG->OpenForOrderOnNPC(this, Npc, MenuScreenPos);
		bOpenedMenuContent = true;
	}

	if (!bOpenedMenuContent && RTSNPCOrderOnNPCMenuInstance)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("[RTS|NPCOrder|NPC] OpenForOrderOnNPC impossible (cast). Classe=%s"),
			*GetNameSafe(RTSNPCOrderOnNPCMenuInstance->GetClass()));
#endif
		RTSNPCOrderOnNPCMenuInstance->RemoveFromParent();
		RTSNPCOrderOnNPCMenuInstance = nullptr;
		return;
	}

	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		if (RTSNPCOrderOnNPCMenuInstance)
		{
			InputMode.SetWidgetToFocus(RTSNPCOrderOnNPCMenuInstance->TakeWidget());
		}
		SetInputMode(InputMode);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
	}

#if !UE_BUILD_SHIPPING
	TheHouseRTSContextDebug::Screen(
		this,
		FString::Printf(TEXT("[RTS|NPCOrder|NPC] Opened (%s) for %s"),
			*GetNameSafe(RTSNPCOrderOnNPCMenuInstance->GetClass()),
			*Npc->GetName()),
		FColor::Green);
#endif

	if (UWorld* WorldForTime = GetWorld())
	{
		LastRTSContextMenuTryOpenTimeSeconds = WorldForTime->GetTimeSeconds();
	}
}

void ATheHousePlayerController::TheHouse_OpenNPCOrderOnGroundMenuShared(const FVector& GroundLocation, const FVector2D& MenuScreenPos)
{
	if (UWorld* WorldForTime = GetWorld())
	{
		const double NowSec = WorldForTime->GetTimeSeconds();
		if (LastRTSContextMenuTryOpenTimeSeconds >= 0.0 &&
			(NowSec - LastRTSContextMenuTryOpenTimeSeconds) < TheHouse_RTSContextMenuTryOpenMinIntervalSeconds)
		{
			return;
		}
	}

	CloseRTSContextMenu();
	ClosePlacedObjectSettingsWidget();

	LastNPCOrderGroundTarget = GroundLocation;

	NPCOrderOnGroundRuntimeOptionDefs.Reset();
	GatherNPCOrderActionsOnGround(NPCOrderOnGroundRuntimeOptionDefs);

	UClass* MenuWidgetClass = NPCOrderContextMenuWidgetClass ? NPCOrderContextMenuWidgetClass.Get()
															 : UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
	if (!MenuWidgetClass->IsChildOf(UTheHouseNPCOrderContextMenuUMGWidget::StaticClass()))
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error,
			TEXT("[RTS|NPCOrder|Ground] NPCOrderContextMenuWidgetClass (%s) doit hériter de UTheHouseNPCOrderContextMenuUMGWidget — fallback C++."),
			*GetNameSafe(MenuWidgetClass));
#endif
		MenuWidgetClass = UTheHouseNPCOrderContextMenuUMGWidget::StaticClass();
	}

	RTSNPCOrderOnGroundMenuInstance = CreateWidget<UUserWidget>(this, MenuWidgetClass);
	if (!RTSNPCOrderOnGroundMenuInstance)
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTS|NPCOrder|Ground] CreateWidget failed"), FColor::Red);
		return;
	}

	RTSNPCOrderOnGroundMenuInstance->SetVisibility(ESlateVisibility::Visible);
	RTSNPCOrderOnGroundMenuInstance->SetRenderOpacity(1.f);
	RTSNPCOrderOnGroundMenuInstance->SetIsEnabled(true);
	RTSNPCOrderOnGroundMenuInstance->AddToViewport(100000);

	bool bOpenedMenuContent = false;
	if (UTheHouseNPCOrderContextMenuUMGWidget* UMG = Cast<UTheHouseNPCOrderContextMenuUMGWidget>(RTSNPCOrderOnGroundMenuInstance))
	{
		UMG->OpenForOrderOnGround(this, GroundLocation, MenuScreenPos);
		bOpenedMenuContent = true;
	}

	if (!bOpenedMenuContent && RTSNPCOrderOnGroundMenuInstance)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error, TEXT("[RTS|NPCOrder|Ground] OpenForOrderOnGround impossible (cast). Classe=%s"),
			*GetNameSafe(RTSNPCOrderOnGroundMenuInstance->GetClass()));
#endif
		RTSNPCOrderOnGroundMenuInstance->RemoveFromParent();
		RTSNPCOrderOnGroundMenuInstance = nullptr;
		return;
	}

	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		if (RTSNPCOrderOnGroundMenuInstance)
		{
			InputMode.SetWidgetToFocus(RTSNPCOrderOnGroundMenuInstance->TakeWidget());
		}
		SetInputMode(InputMode);

		if (FSlateApplication::IsInitialized())
		{
			FSlateApplication::Get().ReleaseAllPointerCapture();
		}
	}

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.5f,
			FColor::Green,
			FString::Printf(TEXT("[RTS|NPCOrder|Ground] Menu ouvert (%s) pos=(%.0f,%.0f) options=%d"),
				*GetNameSafe(RTSNPCOrderOnGroundMenuInstance->GetClass()),
				MenuScreenPos.X, MenuScreenPos.Y,
				NPCOrderOnGroundRuntimeOptionDefs.Num()));
	}
#endif

	if (UWorld* WorldForTime = GetWorld())
	{
		LastRTSContextMenuTryOpenTimeSeconds = WorldForTime->GetTimeSeconds();
	}
}

void ATheHousePlayerController::TryOpenRTSContextMenuAtCursor()
{
	ATheHouseCameraPawn* const RtsPawn = Cast<ATheHouseCameraPawn>(GetPawn());
	if (!RtsPawn)
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTSContext] Ignored: besoin du pawn caméra RTS"), FColor::Silver);
		return;
	}
	if (RTSPawnReference != RtsPawn)
	{
		RTSPawnReference = RtsPawn;
	}
	if (IsRtsCameraRotateModifierPhysicallyDown())
	{
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTSContext] Ignored: Alt held (camera rotate)"), FColor::Silver);
		return;
	}

	TMap<AActor*, FHitResult> Merged;
	CollectMergedRTSLineHitsUnderCursor(Merged);

	ATheHouseNPCCharacter* BestNpc = nullptr;
	float BestNpcDist = TNumericLimits<float>::Max();
	ATheHouseObject* BestObj = nullptr;
	float BestObjDist = TNumericLimits<float>::Max();

	for (const TPair<AActor*, FHitResult>& P : Merged)
	{
		if (ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(P.Key))
		{
			if (Npc->IsSelectable() && P.Value.Distance < BestNpcDist)
			{
				BestNpcDist = P.Value.Distance;
				BestNpc = Npc;
			}
		}

		if (ATheHouseObject* Obj = Cast<ATheHouseObject>(P.Key))
		{
			if (IsValid(Obj) && Obj != PreviewActor && !Obj->IsPlacementPreviewActor() && P.Value.Distance < BestObjDist)
			{
				BestObjDist = P.Value.Distance;
				BestObj = Obj;
			}
		}
	}

	if (BestNpc && BestObj)
	{
		if (BestNpcDist <= BestObjDist)
		{
			BestObj = nullptr;
		}
		else
		{
			BestNpc = nullptr;
		}
	}

	const bool bHasOrderNPCs = HasSelectedNPCsForRTSOrders();

	if (BestNpc)
	{
		const bool bNpcIsInSelection = IsActorInRTSSelection(BestNpc);
		const bool bCanOpenNpcOrderMenu =
			bHasOrderNPCs && (!bNpcIsInSelection || HasRTSSelectionActorOtherThan(BestNpc));

		if (bCanOpenNpcOrderMenu)
		{
			TheHouse_OpenNPCOrderOnNPCMenuShared(BestNpc, TheHouse_GetContextMenuViewportPosition());
			return;
		}

		EnsureDefaultNPCRTSContextMenuDefs();
		TheHouse_OpenRTSContextMenuForNPCShared(BestNpc, TheHouse_GetContextMenuViewportPosition());
		return;
	}

	if (BestObj)
	{
		if (bHasOrderNPCs)
		{
			TheHouse_OpenNPCOrderOnObjectMenuShared(BestObj, TheHouse_GetContextMenuViewportPosition());
			return;
		}

		TheHouse_OpenRTSContextMenuShared(BestObj, TheHouse_GetContextMenuViewportPosition());
		return;
	}

	FVector GroundLoc;
	if (!TryResolveGroundClickLocation(Merged, GroundLoc))
	{
		CloseRTSContextMenu();
		TheHouseRTSContextDebug::Screen(this, TEXT("[RTSContext] Pas de sol / pas de trace sous le curseur"), FColor::Orange);
		return;
	}

	if (bHasOrderNPCs)
	{
		TheHouse_OpenNPCOrderOnGroundMenuShared(GroundLoc, TheHouse_GetContextMenuViewportPosition());
		return;
	}

	IssueMoveOrdersToSelectedNPCsAtLocation(GroundLoc);
	CloseRTSContextMenu();

#if !UE_BUILD_SHIPPING
	TheHouseRTSContextDebug::Screen(
		this,
		FString::Printf(TEXT("[RTSContext] Ordre MoveTo — point (%.0f, %.0f, %.0f)"), GroundLoc.X, GroundLoc.Y, GroundLoc.Z),
		FColor::Cyan);
#endif
}

void ATheHousePlayerController::TryOpenRTSContextMenuForNPC(ATheHouseNPCCharacter* Npc)
{
	ATheHouseCameraPawn* const RtsPawn = Cast<ATheHouseCameraPawn>(GetPawn());
	if (!RtsPawn)
	{
		return;
	}
	if (RTSPawnReference != RtsPawn)
	{
		RTSPawnReference = RtsPawn;
	}
	if (IsRtsCameraRotateModifierPhysicallyDown())
	{
		return;
	}
	if (!IsValid(Npc) || !Npc->IsSelectable())
	{
		return;
	}

	EnsureDefaultNPCRTSContextMenuDefs();
	TheHouse_OpenRTSContextMenuForNPCShared(Npc, TheHouse_GetContextMenuViewportPosition());
}

void ATheHousePlayerController::TryOpenRTSContextMenuForObject(ATheHouseObject* Obj)
{
	ATheHouseCameraPawn* const RtsPawn = Cast<ATheHouseCameraPawn>(GetPawn());
	if (!RtsPawn)
	{
		return;
	}
	if (RTSPawnReference != RtsPawn)
	{
		RTSPawnReference = RtsPawn;
	}
	if (IsRtsCameraRotateModifierPhysicallyDown())
	{
		return;
	}
	if (!IsValid(Obj) || Obj->IsPlacementPreviewActor())
	{
		return;
	}

	TheHouse_OpenRTSContextMenuShared(Obj, TheHouse_GetContextMenuViewportPosition());
}

bool ATheHousePlayerController::TheHouse_IsModalRtsUiBlockingWorldSelection() const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FVector2D AbsMouse = FSlateApplication::Get().GetCursorPos();

	auto IsUnderGeometry = [&](const UUserWidget* W) -> bool
	{
		if (!IsValid(W))
		{
			return false;
		}
		const FGeometry& Geo = W->GetCachedGeometry();
		if (Geo.GetLocalSize().IsNearlyZero())
		{
			return false;
		}
		return USlateBlueprintLibrary::IsUnderLocation(Geo, AbsMouse);
	};

	return IsUnderGeometry(RTSContextMenuInstance) ||
		   IsUnderGeometry(RTSNPCContextMenuInstance) ||
		   IsUnderGeometry(RTSNPCOrderOnObjectMenuInstance) ||
		   IsUnderGeometry(RTSNPCOrderOnNPCMenuInstance) ||
		   IsUnderGeometry(RTSNPCOrderOnGroundMenuInstance) ||
		   IsUnderGeometry(PlacedObjectSettingsWidgetInstance) ||
		   IsUnderGeometry(TheHouseSettingsMenuWidgetInstance);
}

bool ATheHousePlayerController::TheHouse_IsCursorOverBlockingRTSViewportUI() const
{
	if (TheHouse_IsModalRtsUiBlockingWorldSelection())
	{
		return true;
	}

	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FVector2D AbsMouse = FSlateApplication::Get().GetCursorPos();

	// Ne pas utiliser IsUnderLocation(RTSMainWidgetInstance) : la géométrie du root WBP couvre souvent
	// tout l’écran (même zones « vides »), ce qui empêchait bRtsSelectGestureFromWorld, le rectangle HUD,
	// et les clics sur le monde. On ne bloque le panneau principal que si le chemin Slate pointe vers
	// un UMG réellement hit-testable (ESlateVisibility::Visible) dans cet arbre.
	const TArray<TSharedRef<SWindow>> Windows = FSlateApplication::Get().GetInteractiveTopLevelWindows();
	const FWidgetPath Path =
		FSlateApplication::Get().LocateWindowUnderMouse(AbsMouse, Windows, false);
	return TheHouse_SlatePathBlocksRtsWorldSelection(
		Path,
		RTSMainWidgetInstance.Get(),
		RTSContextMenuInstance.Get(),
		RTSNPCContextMenuInstance.Get(),
		PlacedObjectSettingsWidgetInstance.Get(),
		RTSNPCOrderOnObjectMenuInstance.Get(),
		RTSNPCOrderOnNPCMenuInstance.Get(),
		RTSNPCOrderOnGroundMenuInstance.Get(),
		TheHouseSettingsMenuWidgetInstance.Get());
}

void ATheHousePlayerController::Input_CancelOrCloseRts()
{
	TryConsumeEscapeForRtsUi();
}

bool ATheHousePlayerController::TryConsumeEscapeForRtsUi()
{
	bool bHandled = false;

	if (TheHouseSettingsMenuWidgetInstance)
	{
		CloseTheHouseSettingsMenu();
		return true;
	}

	if (AnyRTSContextMenuOpen())
	{
		CloseRTSContextMenu();
		bHandled = true;
	}

	if (PlacedObjectSettingsWidgetInstance)
	{
		ClosePlacedObjectSettingsWidget();
		bHandled = true;
	}

	const bool bRts = (RTSPawnReference && GetPawn() == RTSPawnReference);

	if (bRts && bIsSelecting)
	{
		bIsSelecting = false;
		if (ATheHouseHUD* HUD = Cast<ATheHouseHUD>(GetHUD()))
		{
			HUD->StopSelection();
		}
		bHandled = true;
	}

	if (PlacementState == EPlacementState::Previewing)
	{
		CancelPlacement();
		bHandled = true;
	}

	if (bRts && RTSSelectedActors.Num() > 0)
	{
		RTS_DeselectAll();
		bHandled = true;
	}

	if (bRts && RTSMainWidgetInstance && RTSMainWidgetInstance->EscapeCloseOverlays())
	{
		bHandled = true;
	}

	return bHandled;
}

void ATheHousePlayerController::Input_RTSObjectContext()
{
	// Volontairement vide : l’ouverture au clic droit est traitée uniquement dans
	// TheHouse_PollInputFrame (état physique RMB). Un binding résiduel (Blueprint,
	// Enhanced Input, ancien DefaultInput) sur la même action + TryOpen ici
	// générait des logs [RTSContext] Opened en rafale et un menu instable.
	// Pour ouvrir le menu depuis un BP : appeler TryOpenRTSContextMenuAtCursor().
}

void ATheHousePlayerController::AddMoney(int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	Money = FMath::Max(0, Money + Delta);
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::SetMoney(int32 NewMoney)
{
	Money = FMath::Max(0, NewMoney);
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::SetStoredPlaceableStacks(const TArray<FTheHouseStoredStack>& NewStacks)
{
	StoredPlaceableStacks = NewStacks;
	RefreshRTSMainWidget();
}

int32 ATheHousePlayerController::GetPurchasePriceForClass(TSubclassOf<ATheHouseObject> ObjectClass) const
{
	if (!ObjectClass)
	{
		return 0;
	}
	if (const ATheHouseObject* CDO = ObjectClass.GetDefaultObject())
	{
		return FMath::Max(0, CDO->PurchasePrice);
	}
	return 0;
}

bool ATheHousePlayerController::CanAffordCatalogPurchaseForClass(TSubclassOf<ATheHouseObject> ObjectClass) const
{
	return Money >= GetPurchasePriceForClass(ObjectClass);
}

void ATheHousePlayerController::ClosePlacedObjectSettingsWidget()
{
	PlacedObjectSettingsLastBoundObject.Reset();
	if (PlacedObjectSettingsWidgetInstance)
	{
		PlacedObjectSettingsWidgetInstance->RemoveFromParent();
		PlacedObjectSettingsWidgetInstance = nullptr;
	}
}

void ATheHousePlayerController::ClosePlacedObjectSettingsPanel()
{
	ClosePlacedObjectSettingsWidget();
}

void ATheHousePlayerController::OpenTheHouseSettingsMenu()
{
	if (!TheHouseSettingsMenuWidgetClass)
	{
		return;
	}
	CloseTheHouseSettingsMenu();
	TheHouseSettingsMenuWidgetInstance =
		CreateWidget<UTheHouseSettingsMenuWidget>(this, TheHouseSettingsMenuWidgetClass);
	if (!TheHouseSettingsMenuWidgetInstance)
	{
		return;
	}
	TheHouseSettingsMenuWidgetInstance->BindToPlayerController(this);
	TheHouseSettingsMenuWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	TheHouseSettingsMenuWidgetInstance->AddToViewport(250000);
}

void ATheHousePlayerController::CloseTheHouseSettingsMenu()
{
	if (TheHouseSettingsMenuWidgetInstance)
	{
		TheHouseSettingsMenuWidgetInstance->RemoveFromParent();
		TheHouseSettingsMenuWidgetInstance = nullptr;
	}
}

void ATheHousePlayerController::Input_TheHouseOpenSettings()
{
	if (TheHouseSettingsMenuWidgetInstance)
	{
		CloseTheHouseSettingsMenu();
	}
	else
	{
		OpenTheHouseSettingsMenu();
	}
}

void ATheHousePlayerController::OpenPlacedObjectSettingsWidgetFor(ATheHouseObject* Obj)
{
	if (!IsValid(Obj) || Obj->IsPlacementPreviewActor())
	{
		ClosePlacedObjectSettingsWidget();
		return;
	}
	if (!Obj->bOpenParametersPanelOnLeftClick)
	{
		ClosePlacedObjectSettingsWidget();
		return;
	}
	if (!PlacedObjectSettingsWidgetClass)
	{
		ClosePlacedObjectSettingsWidget();
		return;
	}
	if (!RTSPawnReference || GetPawn() != RTSPawnReference)
	{
		return;
	}

	if (PlacedObjectSettingsWidgetInstance && IsValid(PlacedObjectSettingsWidgetInstance))
	{
		const bool bSameTarget =
			(PlacedObjectSettingsWidgetInstance->GetTargetPlacedObject() == Obj) ||
			(PlacedObjectSettingsLastBoundObject.Get() == Obj);
		if (bSameTarget)
		{
			PlacedObjectSettingsWidgetInstance->SetTargetPlacedObject(Obj);
			PlacedObjectSettingsLastBoundObject = Obj;
			return;
		}
	}

	ClosePlacedObjectSettingsWidget();
	PlacedObjectSettingsWidgetInstance = CreateWidget<UTheHousePlacedObjectSettingsWidget>(
		this,
		PlacedObjectSettingsWidgetClass);
	if (!PlacedObjectSettingsWidgetInstance)
	{
		return;
	}
	PlacedObjectSettingsWidgetInstance->SetTargetPlacedObject(Obj);
	PlacedObjectSettingsLastBoundObject = Obj;
	// Au-dessus du panneau RTS (~10) et du menu contextuel (100000) — un Z trop bas peut laisser le WBP "sous" d'autres couches UMG.
	PlacedObjectSettingsWidgetInstance->SetVisibility(ESlateVisibility::Visible);
	PlacedObjectSettingsWidgetInstance->AddToViewport(200000);
}

void ATheHousePlayerController::RTS_SellPlacedObject(ATheHouseObject* Obj)
{
	if (!IsValid(Obj) || Obj == PreviewActor)
	{
		return;
	}

	if (PlacedObjectSettingsWidgetInstance && PlacedObjectSettingsWidgetInstance->GetTargetPlacedObject() == Obj)
	{
		ClosePlacedObjectSettingsWidget();
	}

	TArray<FIntPoint> Cells;
	GetFootprintCellsForTransform(Obj, Obj->GetActorTransform(), Cells);
	MarkCellsFree(Cells);

	RTSSelectedActors.RemoveAllSwap([&](const TWeakObjectPtr<AActor>& W)
	{
		return W.Get() == Obj;
	});

	if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Obj))
	{
		Sel->OnDeselect();
	}

	const int32 SaleValue = (Obj->SellValue > 0) ? Obj->SellValue : Obj->PurchasePrice;
	AddMoney(FMath::Max(0, SaleValue));
	Obj->Destroy();
}

void ATheHousePlayerController::RTS_StorePlacedObject(ATheHouseObject* Obj)
{
	if (!IsValid(Obj) || Obj == PreviewActor)
	{
		return;
	}

	if (PlacedObjectSettingsWidgetInstance && PlacedObjectSettingsWidgetInstance->GetTargetPlacedObject() == Obj)
	{
		ClosePlacedObjectSettingsWidget();
	}

	TArray<FIntPoint> Cells;
	GetFootprintCellsForTransform(Obj, Obj->GetActorTransform(), Cells);
	MarkCellsFree(Cells);

	RTSSelectedActors.RemoveAllSwap([&](const TWeakObjectPtr<AActor>& W)
	{
		return W.Get() == Obj;
	});

	if (ITheHouseSelectable* Sel = Cast<ITheHouseSelectable>(Obj))
	{
		Sel->OnDeselect();
	}

	AddOrIncrementStoredStock(TSubclassOf<ATheHouseObject>(Obj->GetClass()));
	Obj->Destroy();
}

namespace
{
	// Les défauts C++ utilisent Delete / Store ; si les entrées du tableau ont été mal
	// configurées (OptionId = libellé « Vendre » au lieu de Delete), on accepte des alias.
	bool IsRTSSellContextOption(FName OptionId)
	{
		using namespace TheHouseRTSContextIds;
		return OptionId == DeleteObject
			|| OptionId == FName(TEXT("Vendre"))
			|| OptionId == FName(TEXT("Sell"));
	}

	bool IsRTSStoreContextOption(FName OptionId)
	{
		using namespace TheHouseRTSContextIds;
		return OptionId == StoreObject
			|| OptionId == FName(TEXT("MettreEnStock"))
			|| OptionId == FName(TEXT("Mettre en stock"))
			|| OptionId == FName(TEXT("Stock"));
	}

	bool IsRTSRelocateContextOption(FName OptionId)
	{
		using namespace TheHouseRTSContextIds;
		return OptionId == RelocateObject
			|| OptionId == FName(TEXT("Move"))
			|| OptionId == FName(TEXT("Deplacer"))
			|| OptionId == FName(TEXT("Déplacer"));
	}
}

void ATheHousePlayerController::ExecuteRTSContextMenuOption(FName OptionId, ATheHouseObject* TargetObject)
{
#if !UE_BUILD_SHIPPING
	// Toujours visible dans la sortie PIE (sans activer TheHouse.RTS.ContextMenu.Debug).
	UE_LOG(LogTemp, Warning, TEXT("[RTSContext] ExecuteRTSContextMenuOption: OptionId=%s Target=%s"),
		*OptionId.ToString(), TargetObject ? *TargetObject->GetName() : TEXT("null"));
#endif

	if (!IsValid(TargetObject) || TargetObject == PreviewActor || TargetObject->IsPlacementPreviewActor())
	{
		CloseRTSContextMenu();
		return;
	}

#if !UE_BUILD_SHIPPING
	TheHouseRTSContextDebug::Screen(
		this,
		FString::Printf(TEXT("[RTSContext] Pick option=%s target=%s"), *OptionId.ToString(), *TargetObject->GetName()),
		FColor::Cyan);
#endif

	if (IsRTSRelocateContextOption(OptionId))
	{
		StartRelocationPreviewForPlacedObject(TargetObject);
	}
	else if (IsRTSSellContextOption(OptionId))
	{
		RTS_SellPlacedObject(TargetObject);
	}
	else if (IsRTSStoreContextOption(OptionId))
	{
		RTS_StorePlacedObject(TargetObject);
	}
	else
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("[RTSContext] OptionId non géré : %s (attendu Delete/Store/Relocate ou alias Vendre/…) — HandleRTSContextMenuOption"),
			*OptionId.ToString());
#endif
		HandleRTSContextMenuOption(OptionId, TargetObject);
	}

	CloseRTSContextMenu();
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::ExecuteNPCRTSContextMenuOption(FName OptionId, ATheHouseNPCCharacter* TargetNPC)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Warning, TEXT("[RTSNPCContext] ExecuteNPCRTSContextMenuOption: OptionId=%s Target=%s"),
		*OptionId.ToString(), TargetNPC ? *TargetNPC->GetName() : TEXT("null"));
#endif

	if (!IsValid(TargetNPC))
	{
		CloseRTSContextMenu();
		return;
	}

	using namespace TheHouseNPCContextIds;

	if (OptionId == UsePlacedObjectAtCursor)
	{
		if (ATheHouseObject* ObjUnderCursor = FindPlacedTheHouseObjectUnderCursor())
		{
			if (UTheHouseObjectSlotUserComponent* SlotComp = TargetNPC->FindComponentByClass<UTheHouseObjectSlotUserComponent>())
			{
				SlotComp->UseObjectSlot(ObjUnderCursor, NAME_None, nullptr);
			}
#if !UE_BUILD_SHIPPING
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RTSNPCContext] Pas de UTheHouseObjectSlotUserComponent sur %s — ajoutez le composant au PNJ."),
					*TargetNPC->GetName());
			}
#endif
		}
#if !UE_BUILD_SHIPPING
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[RTSNPCContext] UsePlacedObjectAtCursor : aucun ATheHouseObject sous le curseur."));
		}
#endif
	}
	else
	{
		HandleNPCRTSContextMenuOption(OptionId, TargetNPC);
	}

	CloseRTSContextMenu();
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::NotifyRtsOrderMenuOptionClickedFromUi()
{
	bRtsSuppressNextSelectReleaseWorldPick = true;
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

void ATheHousePlayerController::HandleNPCRTSContextMenuOption_Implementation(FName OptionId, ATheHouseNPCCharacter* TargetNPC)
{
	(void)OptionId;
	(void)TargetNPC;
}

void ATheHousePlayerController::GatherNPCOrderActionsOnObject_Implementation(
	ATheHouseObject* TargetObject,
	TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs)
{
	(void)TargetObject;
	// Par défaut : une entrée « debug » pour que le menu soit visible sans BP.
	if (OutOptionDefs.Num() == 0)
	{
		FTheHouseRTSContextMenuOptionDef D;
		D.OptionId = FName(TEXT("DebugOrder"));
		D.Label = FText::FromString(TEXT("Debug order (override in BP)"));
		OutOptionDefs.Add(D);
	}
}

void ATheHousePlayerController::GatherNPCOrderActionsOnNPC_Implementation(
	ATheHouseNPCCharacter* TargetNPC,
	TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs)
{
	// Règles "garde" :
	// - Les options ne sont proposées que si au moins un garde est sélectionné.
	// - "Attaquer" nécessite au moins un garde sélectionné différent de la cible et déjà possédé
	//   par un ATheHouseNPCAIController (pour recevoir l'ordre).
	bool bAnyGuardSelected = false;
	bool bHasGuardIssuerNotTarget = false;
	{
		TArray<ATheHouseNPCCharacter*> Selected;
		GetSelectedNPCsForRTSOrders(Selected);
		for (ATheHouseNPCCharacter* N : Selected)
		{
			if (!IsValid(N))
			{
				continue;
			}
			const bool bIsGuard = N->IsStaffGuardNPC();
			bAnyGuardSelected = bAnyGuardSelected || bIsGuard;
			if (bIsGuard && N != TargetNPC && Cast<ATheHouseNPCAIController>(N->GetController()) != nullptr)
			{
				bHasGuardIssuerNotTarget = true;
			}
		}
	}

	if (bAnyGuardSelected && IsValid(TargetNPC))
	{
		const ETheHouseNPCCategory TargetCat = ITheHouseNPCIdentity::Execute_GetNPCCategory(TargetNPC);
		if (TargetCat == ETheHouseNPCCategory::Customer)
		{
			FTheHouseRTSContextMenuOptionDef Eject;
			Eject.OptionId = FName(TEXT("GuardEjectCustomerFromZone"));
			Eject.Label = FText::FromString(TEXT("Éjecter de la zone (client)"));
			OutOptionDefs.Add(Eject);
		}

		if (bHasGuardIssuerNotTarget)
		{
			FTheHouseRTSContextMenuOptionDef Atk;
			Atk.OptionId = FName(TEXT("GuardAttackNPC"));
			Atk.Label = FText::FromString(TEXT("Attaquer (ordre garde)"));
			OutOptionDefs.Add(Atk);
		}
	}

	if (OutOptionDefs.Num() == 0)
	{
		FTheHouseRTSContextMenuOptionDef D;
		D.OptionId = FName(TEXT("DebugOrder"));
		D.Label = FText::FromString(TEXT("Debug order (override in BP)"));
		OutOptionDefs.Add(D);
	}
}

void ATheHousePlayerController::ExecuteNPCOrderOnObjectAction_Implementation(FName OptionId, ATheHouseObject* TargetObject)
{
#if !UE_BUILD_SHIPPING
	UE_LOG(LogTemp, Log,
		TEXT("[RTS|NPCOrder|Obj] Execute=%s Target=%s — surcharger ExecuteNPCOrderOnObjectAction en BP / C++ pour la logique."),
		*OptionId.ToString(),
		TargetObject ? *TargetObject->GetName() : TEXT("null"));
#endif
	(void)OptionId;
	(void)TargetObject;
	CloseRTSContextMenu();
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::ExecuteNPCOrderOnNPCAction_Implementation(FName OptionId, ATheHouseNPCCharacter* TargetNPC)
{
	UWorld* World = GetWorld();

	if (OptionId == FName(TEXT("GuardEjectCustomerFromZone")))
	{
		if (IsValid(TargetNPC) && ITheHouseNPCIdentity::Execute_GetNPCCategory(TargetNPC) == ETheHouseNPCCategory::Customer
			&& World)
		{
			if (ATheHouseNPCEjectRegionVolume* Zone = TheHouseNPCOrdersInternal::FindSmallestEjectVolumeContaining(
					World,
					TargetNPC->GetActorLocation()))
			{
				FVector ExitWorld;
				if (Zone->TryComputeEjectionExitWorldLocation(TargetNPC->GetActorLocation(), ExitWorld))
				{
					TheHouseNPCOrdersInternal::ProjectPointToNavOrKeep(World, ExitWorld, ExitWorld);
					if (AAIController* CustAI = Cast<AAIController>(TargetNPC->GetController()))
					{
						FAIMoveRequest Req;
						Req.SetGoalLocation(ExitWorld);
						Req.SetAcceptanceRadius(80.f);
						Req.SetUsePathfinding(true);
						Req.SetAllowPartialPath(true);
						CustAI->MoveTo(Req);
					}
				}
			}
#if !UE_BUILD_SHIPPING
			else
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[RTS|Guard] Éjection : aucun TheHouseNPCEjectRegionVolume ne contient la position de %s."),
					*TargetNPC->GetName());
			}
#endif
		}
	}
	else if (OptionId == FName(TEXT("GuardAttackNPC")))
	{
		if (IsValid(TargetNPC))
		{
			TArray<ATheHouseNPCCharacter*> Selected;
			GetSelectedNPCsForRTSOrders(Selected);
			for (ATheHouseNPCCharacter* Guard : Selected)
			{
				if (!IsValid(Guard) || !Guard->IsStaffGuardNPC() || Guard == TargetNPC)
				{
					continue;
				}
				if (ATheHouseNPCAIController* GAI = Cast<ATheHouseNPCAIController>(Guard->GetController()))
				{
					GAI->IssueAttackOrderOnScriptedTarget(TargetNPC);
				}
			}
		}
	}
#if !UE_BUILD_SHIPPING
	else
	{
		UE_LOG(LogTemp, Log,
			TEXT("[RTS|NPCOrder|NPC] Execute=%s Target=%s — surcharger ExecuteNPCOrderOnNPCAction en BP / C++ pour la logique."),
			*OptionId.ToString(),
			TargetNPC ? *TargetNPC->GetName() : TEXT("null"));
	}
#endif

	CloseRTSContextMenu();
	RefreshRTSMainWidget();
}

void ATheHousePlayerController::GatherNPCOrderActionsOnGround_Implementation(TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs)
{
	// Par défaut : un ordre MoveTo vers LastNPCOrderGroundTarget + une entrée debug.
	FTheHouseRTSContextMenuOptionDef Move;
	Move.OptionId = FName(TEXT("MoveTo"));
	Move.Label = FText::FromString(TEXT("Se déplacer ici"));
	OutOptionDefs.Add(Move);

	if (OutOptionDefs.Num() == 1)
	{
		FTheHouseRTSContextMenuOptionDef D;
		D.OptionId = FName(TEXT("DebugOrder"));
		D.Label = FText::FromString(TEXT("Debug order (override in BP)"));
		OutOptionDefs.Add(D);
	}
}

void ATheHousePlayerController::ExecuteNPCOrderOnGroundAction_Implementation(FName OptionId)
{
	if (OptionId == FName(TEXT("MoveTo")))
	{
		IssueMoveOrdersToSelectedNPCsAtLocation(LastNPCOrderGroundTarget);
	}
#if !UE_BUILD_SHIPPING
	else
	{
		UE_LOG(LogTemp, Log,
			TEXT("[RTS|NPCOrder|Ground] Execute=%s Ground=%s — surcharger ExecuteNPCOrderOnGroundAction en BP / C++."),
			*OptionId.ToString(),
			*LastNPCOrderGroundTarget.ToString());
	}
#endif

	CloseRTSContextMenu();
	RefreshRTSMainWidget();
}
