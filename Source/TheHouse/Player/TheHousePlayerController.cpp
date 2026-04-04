// Fill out your copyright notice in the Description page of Project Settings.


#include "TheHousePlayerController.h"
#include "TheHouseCameraPawn.h"
#include "TheHouseFPSCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "TheHouse/Core/TheHouseHUD.h"
#include "TheHouse/Core/TheHouseSelectable.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "TimerManager.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
/** En Game+UI le focus clavier reste souvent sur un widget Slate : ZQSD n’atteignent pas le jeu. */
void TheHouse_FocusKeyboardOnGameViewport()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().SetAllUserFocusToGameViewport(EFocusCause::SetDirectly);
	}
}
} // namespace

namespace TheHouseInputPoll
{
	/** PlayerInput::IsPressed peut rester faux (Enhanced Input / focus Slate) ; la vue jeu reflète l’état matériel. */
	bool IsKeyPhysicallyDown(const APlayerController* PC, FKey Key)
	{
		if (!PC)
		{
			return false;
		}
		if (PC->IsInputKeyDown(Key))
		{
			return true;
		}
		if (UWorld* World = PC->GetWorld())
		{
			if (UGameViewportClient* GVC = World->GetGameViewport())
			{
				if (GVC->Viewport && GVC->Viewport->KeyState(Key))
				{
					return true;
				}
			}
		}
		return false;
	}

	float PollMoveForward(const APlayerController* PC)
	{
		float v = 0.f;
		if (IsKeyPhysicallyDown(PC, EKeys::W) || IsKeyPhysicallyDown(PC, EKeys::Z))
		{
			v += 1.f;
			FString DebugMsg = FString::Printf(TEXT("Forward V value : %f"), v);
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, DebugMsg);
		}
		if (IsKeyPhysicallyDown(PC, EKeys::S))
		{
			v -= 1.f;
			FString DebugMsg = FString::Printf(TEXT("Forward V value : %f"), v);
			// On l'affiche (Note : on utilise %f pour un float, pas %d qui est pour les entiers)
			GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, DebugMsg);
		}
		return FMath::Clamp(v, -1.f, 1.f);
	}

	float PollMoveRight(const APlayerController* PC)
	{
		float v = 0.f;
		if (IsKeyPhysicallyDown(PC, EKeys::D))
		{
			v += 1.f;
		}
		if (IsKeyPhysicallyDown(PC, EKeys::A) || IsKeyPhysicallyDown(PC, EKeys::Q))
		{
			v -= 1.f;
		}
		return FMath::Clamp(v, -1.f, 1.f);
	}
} // namespace TheHouseInputPoll

void ATheHousePlayerController::TheHouse_ApplyWheelZoom(float WheelDelta)
{
	if (FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}
	// Valeur passée à Zoom() : NewLength += Value * RTSZoomSpeed * (-1). Souris haute précision : |WheelDelta| très petit.
	float V = WheelDelta * RTSWheelZoomMultiplier;
	const float ArmDeltaIfApplied = FMath::Abs(V * RTSZoomSpeed);
	if (ArmDeltaIfApplied < RTSMinArmDeltaPerWheelEvent)
	{
		V = FMath::Sign(V) * (RTSMinArmDeltaPerWheelEvent / FMath::Max(RTSZoomSpeed, 1.f));
	}
	Zoom(V);
}

ATheHousePlayerController::ATheHousePlayerController()
{
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

void ATheHousePlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Listen server / edge init: IsLocallyControlled() chains to IsLocalController(); if bIsLocalPlayerController
	// is not set yet, CharacterMovement will not run ControlledCharacterMove and WASD polling in Tick is skipped.
	if (Cast<ULocalPlayer>(Player))
	{
		SetAsLocalPlayerController();
	}

	// Garantit PlayerInput avant le premier Tick (sinon IsInputKeyDown reste faux pour ZQSD / pavé num.).
	if (Cast<ULocalPlayer>(Player) && PlayerInput == nullptr)
	{
		InitInputSystem();
	}

	if (!IsValid(RTSPawnReference))
	{
		RTSPawnReference = Cast<ATheHouseCameraPawn>(GetPawn());
	}

	if (IsValid(RTSPawnReference) && RTSPawnReference->CameraBoom)
	{
		TargetZoomLength = RTSPawnReference->CameraBoom->TargetArmLength;
	}

	// RTS : yaw sur le contrôleur ; pitch sur le spring arm (voir ATheHouseCameraPawn::RTSArmPitchDegrees).
	SetControlRotation(FRotator(0.f, 0.f, 0.f));
	if (IsValid(RTSPawnReference))
	{
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
		TEXT("[TheHouse] Révision entrées 2026-04-03 | PlayerInput=%s | Si tu vois « EnhancedPlayerInput », DefaultInput.ini est encore en Enhanced — ZQSD/zoom resteront cassés."),
		PlayerInput ? *PlayerInput->GetClass()->GetName() : TEXT("null"));

	UE_LOG(LogTemp, Warning, TEXT("[TheHouse] Contrôleur joueur (BeginPlay) | classe=%s | pion=%s | LocalPlayer=%s | LocalController=%s"),
		*GetClass()->GetName(),
		GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("(aucun)"),
		IsLocalPlayerController() ? TEXT("oui") : TEXT("non"),
		IsLocalController() ? TEXT("oui") : TEXT("non"));

#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			14.f,
			FColor::Cyan,
			FString::Printf(
				TEXT("[TheHouse] Entrées = timer (pas seulement Tick) | %s | Pion : %s | Si rien ne change : BP Event Tick doit appeler Parent, ou Mode de jeu ≠ TheHouse."),
				*GetClass()->GetName(),
				GetPawn() ? *GetPawn()->GetClass()->GetName() : TEXT("(aucun)")));
	}
#endif

	TheHouse_StartInputPollTimer();
}

void ATheHousePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	TheHouse_StartInputPollTimer();
}

void ATheHousePlayerController::TheHouse_StartInputPollTimer()
{
	if (!Cast<ULocalPlayer>(Player) || !GetWorld())
	{
		return;
	}
	if (GetWorldTimerManager().IsTimerActive(TheHouseInputPollTimer))
	{
		return;
	}
	GetWorldTimerManager().SetTimer(
		TheHouseInputPollTimer,
		this,
		&ATheHousePlayerController::TheHouse_PollInputFrame,
		1.f / 120.f,
		true);
	UE_LOG(LogTemp, Warning, TEXT("[TheHouse] Timer d'entrées démarré (120 Hz) sur %s | LocalController=%s"),
		*GetClass()->GetName(),
		IsLocalController() ? TEXT("oui") : TEXT("non"));
}

void ATheHousePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(TheHouseInputPollTimer);
	}
	Super::EndPlay(EndPlayReason);
}

void ATheHousePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	SetIgnoreMoveInput(false);

	if (ACharacter* CharacterPawn = Cast<ACharacter>(InPawn))
	{
		if (UCharacterMovementComponent* MoveComp = CharacterPawn->GetCharacterMovement())
		{
			MoveComp->SetActive(true);
		}
	}

	if (ATheHouseCameraPawn* Cam = Cast<ATheHouseCameraPawn>(InPawn))
	{
		RTSPawnReference = Cam;
		if (Cam->CameraBoom)
		{
			TargetZoomLength = Cam->CameraBoom->TargetArmLength;
		}
	}

	TheHouse_StartInputPollTimer();
}

void ATheHousePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Secours si BeginPlay/ReceivedPlayer n’ont pas lancé le timer (BP sans Super, ordre d’init).
	if (GetWorld() && Cast<ULocalPlayer>(Player) && !GetWorldTimerManager().IsTimerActive(TheHouseInputPollTimer))
	{
		TheHouse_StartInputPollTimer();
	}

	// Déplacement + zoom (ignorés si move input coupé — le pitch RTS reste géré après).
	if (Cast<ULocalPlayer>(Player) && GetPawn() && !IsMoveInputIgnored())
	{
		float Fwd = 0.f;
		float Rt = 0.f;
		if (PlayerInput)
		{
			Fwd = GetInputAxisValue(TEXT("MoveForward"));
			Rt = GetInputAxisValue(TEXT("MoveRight"));
		}
		if (FMath::Abs(Fwd) < 0.02f && FMath::Abs(Rt) < 0.02f)
		{
			Fwd = TheHouseInputPoll::PollMoveForward(this);
			Rt = TheHouseInputPoll::PollMoveRight(this);
		}
		if (!FMath::IsNearlyZero(Fwd))
		{
			MoveForward(Fwd);
		}
		if (!FMath::IsNearlyZero(Rt))
		{
			MoveRight(Rt);
		}

		if (RTSZoomInterpSpeed > KINDA_SMALL_NUMBER)
		{
			if (ATheHouseCameraPawn* Cam = Cast<ATheHouseCameraPawn>(GetPawn()))
			{
				if (Cam->CameraBoom && !FMath::IsNearlyEqual(Cam->CameraBoom->TargetArmLength, TargetZoomLength, 0.25f))
				{
					Cam->CameraBoom->TargetArmLength = FMath::FInterpTo(
						Cam->CameraBoom->TargetArmLength,
						TargetZoomLength,
						DeltaTime,
						RTSZoomInterpSpeed);
				}
			}
		}
	}

	// Pitch RTS (Alt / clic droit) : l’axe LookUp peut rester à 0 ; avec souris capturée, GetMousePosition
	// ne bouge presque pas (curseur recentré) → pas de pitch. GetInputMouseDelta garde le delta brut.
	if (Cast<ULocalPlayer>(Player) && bIsRotateModifierDown)
	{
		if (ATheHouseCameraPawn* Rts = Cast<ATheHouseCameraPawn>(GetPawn()))
		{
			float AxisLook = 0.f;
			if (PlayerInput)
			{
				AxisLook = GetInputAxisValue(TEXT("LookUp"));
			}
			// -1 : même mapping que le FPS (souris vers le haut = relever la vue). Sans ça, le boom + capture souris donnent l’axe Y inversé.
			constexpr float RTSPitchVerticalSign = -1.f;
			if (FMath::Abs(AxisLook) > 0.0001f)
			{
				Rts->ApplyRTSArmPitchDelta(RTSPitchVerticalSign * AxisLook * RotationSensitivity);
			}
			else
			{
				float RawDX = 0.f, RawDY = 0.f;
				GetInputMouseDelta(RawDX, RawDY);
				if (!FMath::IsNearlyZero(RawDY))
				{
					Rts->ApplyRTSArmPitchDelta(RTSPitchVerticalSign * RawDY * 0.25f * RotationSensitivity);
				}
			}
		}
	}
}

void ATheHousePlayerController::TheHouse_PollInputFrame()
{
	// Même critère que le démarrage du timer (IsLocalPlayerController exige IsLocalController ; peut être faux trop tôt).
	if (!Cast<ULocalPlayer>(Player) || !GetPawn())
	{
		return;
	}

	// Déplacement + souris : BindAxis + DefaultInput.ini + DefaultPlayerInputClass=PlayerInput (obligatoire).
	// Le polling ZQSD ici doublait ou restait à 0 selon focus / layout ; GetInputMouseDelta ignorait la sensibilité moteur (0.07) → sens « bugué ».

	// Zoom RTS de secours — Pavé num. + / -
	if (Cast<ATheHouseCameraPawn>(GetPawn()))
	{
		// ~8 uu de variation de bras par frame (ralenti vs ancien 15 ; indépendant de RTSZoomSpeed via la formule).
		const float ZoomKeyStep = 8.f / FMath::Max(RTSZoomSpeed, 1.f);
		if (TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::Add))
		{
			Zoom(ZoomKeyStep);
		}
		if (TheHouseInputPoll::IsKeyPhysicallyDown(this, EKeys::Subtract))
		{
			Zoom(-ZoomKeyStep);
		}
	}

	if (bIsSelecting)
	{
		float MouseX, MouseY;
		if (GetMousePosition(MouseX, MouseY))
		{
			if (ATheHouseHUD* HUD = Cast<ATheHouseHUD>(GetHUD()))
			{
				HUD->UpdateSelection(FVector2D(MouseX, MouseY));
			}
		}
	}
}

void ATheHousePlayerController::SetupInputComponent()
{
	// Force classic UInputComponent: project INI / editor often reset to EnhancedInputComponent, which breaks legacy BindAction here.
	if (InputComponent == nullptr)
	{
		InputComponent = NewObject<UInputComponent>(this, UInputComponent::StaticClass(), TEXT("PC_InputComponent0"));
		InputComponent->RegisterComponent();
	}

	APlayerController::SetupInputComponent();

	// MoveForward/MoveRight : appliqués dans Tick (axes + repli ZQSD) pour que le FPS reçoive le déplacement comme le RTS.
	InputComponent->BindAxis(TEXT("Turn"), this, &ATheHousePlayerController::RotateCamera);
	InputComponent->BindAxis(TEXT("LookUp"), this, &ATheHousePlayerController::LookUp);

	// Actions
	InputComponent->BindAction("DebugToggleFPS", IE_Pressed, this, &ATheHousePlayerController::DebugSwitchToFPS);
	InputComponent->BindAction("RotateModifier", IE_Pressed, this, &ATheHousePlayerController::OnRotateModifierPressed);
	InputComponent->BindAction("RotateModifier", IE_Released, this, &ATheHousePlayerController::OnRotateModifierReleased);
}

#pragma endregion

#pragma region Input Handlers

void ATheHousePlayerController::OnRotateModifierPressed()
{
	bIsRotateModifierDown = true;

	// In RTS Mode, HIDE cursor and LOCK it to allow infinite scrolling/rotation without hitting edge
	if (Cast<ATheHouseCameraPawn>(GetPawn()))
	{
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		TheHouse_FocusKeyboardOnGameViewport();
	}
}

void ATheHousePlayerController::OnRotateModifierReleased()
{
	bIsRotateModifierDown = false;

	// In RTS Mode, SHOW cursor and UNLOCK it to allow clicking UI/Selection
	if (Cast<ATheHouseCameraPawn>(GetPawn()))
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		TheHouse_FocusKeyboardOnGameViewport();
	}
}

void ATheHousePlayerController::MoveForward(float Value)
{
	if (Value == 0.0f) return;

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ACharacter* Char = Cast<ACharacter>(ControlledPawn))
		{
			// Même logique que la caméra RTS : direction = yaw du contrôleur (FPS fiable même si capsule / mesh décalés).
			const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
			const FVector FwdDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
			Char->AddMovementInput(FwdDir, Value, true);
		}
		else if (ATheHouseCameraPawn* Rts = Cast<ATheHouseCameraPawn>(ControlledPawn))
		{
			// Camera-Relative Forward/Back
			// We use the Controller's Yaw because the CameraBoom inherits it.
			FRotator YawRotation(0, GetControlRotation().Yaw, 0);
			FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
			
			// Standard "W = Forward" logic. 
			// If previous inversion was due to bad orientation, this fixes it naturally.
			Rts->AddMovementInput(Direction, Value, true);
		}
	}
}

void ATheHousePlayerController::MoveRight(float Value)
{
	if (Value == 0.0f) return;

	APawn* ControlledPawn = GetPawn();
	if (ControlledPawn)
	{
		if (ACharacter* Char = Cast<ACharacter>(ControlledPawn))
		{
			Char->AddMovementInput(Char->GetActorRightVector(), Value, true);
		}
		else if (ATheHouseCameraPawn* Rts = Cast<ATheHouseCameraPawn>(ControlledPawn))
		{
			// Camera-Relative Right/Left
			FRotator YawRotation(0, GetControlRotation().Yaw, 0);
			FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
			
			// Standard RTS Movement (No Inversion)
			Rts->AddMovementInput(Direction, Value, true);
		}
	}
}

void ATheHousePlayerController::Zoom(float Value)
{
	if (Value == 0.0f) return;

	ATheHouseCameraPawn* Cam = Cast<ATheHouseCameraPawn>(GetPawn());
	if (!IsValid(Cam) && IsValid(RTSPawnReference) && GetPawn() == RTSPawnReference)
	{
		Cam = RTSPawnReference;
	}
	if (!IsValid(Cam) || !Cam->CameraBoom)
	{
		return;
	}

	const float OldLength = Cam->CameraBoom->TargetArmLength;
	float ScaledValue = Value;
	if (bRTSZoomScalesWithArmLength && RTSZoomArmLengthRef > KINDA_SMALL_NUMBER)
	{
		// Vue très éloignée : un cran de molette change nettement la distance perçue.
		ScaledValue *= FMath::Clamp(OldLength / RTSZoomArmLengthRef, 0.5f, 6.f);
	}
	const float NewLength = FMath::Clamp(OldLength + (ScaledValue * RTSZoomSpeed * -1.f), 200.f, 4000.f);
	TargetZoomLength = NewLength;
	if (RTSZoomInterpSpeed <= KINDA_SMALL_NUMBER)
	{
		Cam->CameraBoom->TargetArmLength = NewLength;
	}

#if !UE_BUILD_SHIPPING
	if (bDebugLogRTSZoom && Cam->CameraBoom)
	{
		const float Effective = FVector::Distance(Cam->GetActorLocation(), Cam->RTSCameraComponent ? Cam->RTSCameraComponent->GetComponentLocation() : Cam->GetActorLocation());
		UE_LOG(LogTemp, Log,
			TEXT("[TheHouse] Zoom | TargetArm=%.0f→%.0f | interp=%.1f | boomCollision=%s | dist cam≈%.0f"),
			OldLength,
			NewLength,
			RTSZoomInterpSpeed,
			Cam->CameraBoom->bDoCollisionTest ? TEXT("oui") : TEXT("non"),
			Effective);
	}
#endif

	// Zoom « vers le curseur » le long de l’axe de visée. L’ancien pan avec ToHit.Z=0 forçait l’horizontal monde :
	// combiné au boom incliné, le ressenti était surtout un zoom « vertical » au lieu de suivre l’angle caméra.
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit) && OldLength > KINDA_SMALL_NUMBER && Cam->RTSCameraComponent)
	{
		const FVector CamLoc = Cam->RTSCameraComponent->GetComponentLocation();
		const FVector ViewFwd = Cam->RTSCameraComponent->GetForwardVector().GetSafeNormal();
		const float ZoomDelta = OldLength - NewLength;
		const float Fraction = ZoomDelta / OldLength;
		const float DepthAlongView = FVector::DotProduct(Hit.ImpactPoint - CamLoc, ViewFwd);
		if (DepthAlongView > KINDA_SMALL_NUMBER)
		{
			const float Depth = FMath::Max(DepthAlongView, 50.f);
			FVector Pan = ViewFwd * (Depth * Fraction);
			if (RTSCursorZoomMaxPanPerEvent > 0.f)
			{
				const float M = RTSCursorZoomMaxPanPerEvent;
				if (Pan.SizeSquared() > M * M)
				{
					Pan = Pan.GetSafeNormal() * M;
				}
			}
			Cam->AddActorWorldOffset(Pan);
		}
	}
}

void ATheHousePlayerController::RotateCamera(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}
	// Value intègre déjà MouseX × sensibilité projet (DefaultInput / AxisConfig ~0.07).
	if (Cast<ATheHouseCameraPawn>(GetPawn()))
	{
		if (!bIsRotateModifierDown)
		{
			return;
		}
		AddYawInput(Value * RotationSensitivity);
	}
	else if (Cast<ACharacter>(GetPawn()) && !bShowMouseCursor)
	{
		AddYawInput(Value * RotationSensitivity * 0.5f);
	}
}

void ATheHousePlayerController::LookUp(float Value)
{
	if (FMath::IsNearlyZero(Value))
	{
		return;
	}
	// RTS : le pitch est appliqué dans Tick (axe LookUp + repli GetInputMouseDelta si la souris est capturée).
	if (Cast<ATheHouseCameraPawn>(GetPawn()))
	{
		return;
	}
	if (Cast<ACharacter>(GetPawn()) && !bShowMouseCursor)
	{
		AddPitchInput(Value * RotationSensitivity * 0.5f);
	}
}

#pragma endregion

#pragma region Mode Switching

void ATheHousePlayerController::SwitchToRTS()
{
	if (!IsValid(RTSPawnReference))
	{
		UE_LOG(LogTemp, Warning, TEXT("RTS Pawn Reference is missing!"));
		return;
	}

	// 1. Sync RTS Camera to FPS position
	if (IsValid(ActiveFPSCharacter))
	{
		FVector FPSLoc = ActiveFPSCharacter->GetActorLocation();
		RTSPawnReference->SetActorLocation(FPSLoc + FVector(0.f, 0.f, 150.f));
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
	if (IsValid(ActiveFPSCharacter))
	{
		ActiveFPSCharacter->Destroy();
		ActiveFPSCharacter = nullptr;
	}

	// 4. Input Mode (Show Cursor)
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways); // Keep mouse in window!
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	TheHouse_FocusKeyboardOnGameViewport();
}

void ATheHousePlayerController::SwitchToFPS(FVector TargetLocation, FRotator TargetRotation)
{
	if (!FPSCharacterClass)
	{
		FPSCharacterClass = ATheHouseFPSCharacter::StaticClass();
	}

	// 1. Spawn FPS Character
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	
	ActiveFPSCharacter = GetWorld()->SpawnActor<ATheHouseFPSCharacter>(FPSCharacterClass, TargetLocation, TargetRotation, SpawnParams);

	if (ActiveFPSCharacter)
	{
		// 2. Possess
		Possess(ActiveFPSCharacter);
		SetIgnoreMoveInput(false);

		if (UCharacterMovementComponent* MoveComp = ActiveFPSCharacter->GetCharacterMovement())
		{
			MoveComp->SetMovementMode(MOVE_Walking);
		}

		// 3. Input Mode (Hide Cursor)
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		TheHouse_FocusKeyboardOnGameViewport();
	}
}

void ATheHousePlayerController::DebugSwitchToFPS()
{
	APawn* CurrentPawn = GetPawn();
	
	// Toggle Logic
	if (Cast<ACharacter>(CurrentPawn) && !Cast<ATheHouseCameraPawn>(CurrentPawn))
	{
		SwitchToRTS();
	}
	else if (IsValid(RTSPawnReference))
	{
		// RTS -> FPS
		FVector SpawnLoc = RTSPawnReference->GetActorLocation();
		
		// Use Camera Direction (Yaw) for Spawn
		FRotator CurrentControlRot = GetControlRotation();
		FRotator SpawnRot(0, CurrentControlRot.Yaw, 0); 
		
		SwitchToFPS(SpawnLoc, SpawnRot);
	}
}

void ATheHousePlayerController::Input_SelectPressed()
{
	if (!RTSPawnReference || GetPawn() != RTSPawnReference) return;

	// Start Selection
	float MouseX, MouseY;
	if (GetMousePosition(MouseX, MouseY))
	{
		bIsSelecting = true;
		SelectionStartPos = FVector2D(MouseX, MouseY);

		// Notify HUD
		if (ATheHouseHUD* HUD = Cast<ATheHouseHUD>(GetHUD()))
		{
			HUD->StartSelection(SelectionStartPos);
		}
	}
}

void ATheHousePlayerController::Input_SelectReleased()
{
	bIsSelecting = false;

	// Notify HUD to stop drawing
	if (ATheHouseHUD* HUD = Cast<ATheHouseHUD>(GetHUD()))
	{
		HUD->StopSelection();
	}

	if (!IsValid(RTSPawnReference) || GetPawn() != RTSPawnReference) return;

	float MouseX, MouseY;
	if (!GetMousePosition(MouseX, MouseY)) return;

	FVector2D CurrentMousePos(MouseX, MouseY);
	float DragDistance = FVector2D::Distance(SelectionStartPos, CurrentMousePos);

	// Log for debug
	// UE_LOG(LogTemp, Log, TEXT("Selection Released. Drag Dist: %f"), DragDistance);

	if (DragDistance < 10.0f) // Small movement = Single Click
	{
		// Single Click Selection
		// KEY POINT: We use ECC_Visibility. 
		// Our SmartWall disables its Visibility collision when faded, so this trace will naturally ignore it!
		FHitResult Hit;
		if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
		{
			AActor* HitActor = Hit.GetActor();
			
			// Check Interface AND IsSelectable condition
			// We access the interface via the ITheHouseSelectable pointer to call the virtual function correctly
			if (HitActor && HitActor->Implements<UTheHouseSelectable>())
			{
				ITheHouseSelectable* Selectable = Cast<ITheHouseSelectable>(HitActor);
				if (Selectable && Selectable->IsSelectable())
				{
					// Deselect all others first (Simple logic for now)
					// TODO: Implement Multi-Select with Shift
					// For now, just Log hitting a selectable
					UE_LOG(LogTemp, Warning, TEXT("Selected Actor: %s"), *HitActor->GetName());
					
					Selectable->OnSelect();
				}
			}
		}
	}
	else // Box Selection
	{
		TArray<AActor*> SelectedActors;
		
		if (AHUD* HUD = GetHUD())
		{
			HUD->GetActorsInSelectionRectangle<AActor>(SelectionStartPos, CurrentMousePos, SelectedActors, false, false);
		}

		for (AActor* Actor : SelectedActors)
		{
			// Check Interface AND IsSelectable condition
			if (Actor && Actor->Implements<UTheHouseSelectable>())
			{
				ITheHouseSelectable* Selectable = Cast<ITheHouseSelectable>(Actor);
				if (Selectable && Selectable->IsSelectable())
				{
					UE_LOG(LogTemp, Warning, TEXT("Box Selected: %s"), *Actor->GetName());
					Selectable->OnSelect();
				}
			}
		}
	}
}

#pragma endregion
