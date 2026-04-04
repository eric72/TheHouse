// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TheHousePlayerController.generated.h"

class ATheHouseCameraPawn;
class ATheHouseFPSCharacter;

/**
 * Manages the transition between RTS Camera and FPS Character.
 */
UCLASS()
class THEHOUSE_API ATheHousePlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ATheHousePlayerController();

	/** Molette RTS : appelé par UTheHouseGameViewportClient (molette souvent bloquée avant PlayerInput en Game+UI). */
	void TheHouse_ApplyWheelZoom(float WheelDelta);

protected:
	virtual void BeginPlay() override;
	virtual void ReceivedPlayer() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void SetupInputComponent() override;

public:
	/** Switch to RTS Mode */
	UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
	void SwitchToRTS();

	/** Switch to FPS Mode at specific locations */
	UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
	void SwitchToFPS(FVector TargetLocation, FRotator TargetRotation);

	/** Helper to switch to FPS at current cursor/camera location (for testing) */
	UFUNCTION(BlueprintCallable, Exec, Category = "Camera System")
	void DebugSwitchToFPS();

protected:
	/** Reference to the RTS Camera Pawn (persistent) */
	UPROPERTY()
	ATheHouseCameraPawn* RTSPawnReference;

	/** Reference to the current FPS Character (if alive) */
	UPROPERTY()
	ATheHouseFPSCharacter* ActiveFPSCharacter;

	/** Class to spawn for FPS mode. Set this in BP to the BP_TheHouseFPSCharacter if you use one. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Camera System")
	TSubclassOf<ATheHouseFPSCharacter> FPSCharacterClass;

	// --- Input Functions ---
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Zoom(float Value);
	void RotateCamera(float Value); // Yaw
	void LookUp(float Value);       // Pitch

	// Selection
	UFUNCTION()
	void Input_SelectPressed();
	UFUNCTION()
	void Input_SelectReleased();

	// Modifier State
	bool bIsRotateModifierDown;
	void OnRotateModifierPressed();
	void OnRotateModifierReleased();

	// Selection State
	bool bIsSelecting;
	FVector2D SelectionStartPos;

	/** Entrée déplacée hors de Actor::Tick : un BP « Event Tick » sans Parent bloque tout le Tick C++. */
	FTimerHandle TheHouseInputPollTimer;
	void TheHouse_StartInputPollTimer();
	void TheHouse_PollInputFrame();

protected:
	/** Current Target Zoom Length for interpolation */
	float TargetZoomLength;

	/** Mouse Rotation Sensitivity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
	float RotationSensitivity = 2.0f;

	/** Multiplicateur du delta brut molette avant application (voir aussi RTSMinArmDeltaPerWheelEvent). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "0.25", UIMin = "0.25"))
	float RTSWheelZoomMultiplier = 4.f;

	/** Échelle : variation de TargetArmLength = |Value| * RTSZoomSpeed (uu par « unité » de Value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "50", UIMin = "50"))
	float RTSZoomSpeed = 260.f;

	/** Plancher : variation minimale du spring arm (uu) par événement molette — sans ça, souris haute précision ≈ quelques cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "20", UIMin = "20"))
	float RTSMinArmDeltaPerWheelEvent = 95.f;

	/** Quand le spring arm est long, augmente l’effet d’un cran de molette (vue éloignée). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System")
	bool bRTSZoomScalesWithArmLength = true;

	/** Référence pour bRTSZoomScalesWithArmLength (uu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "100", UIMin = "100"))
	float RTSZoomArmLengthRef = 1000.f;

	/** 0 = illimité. Sinon, limite le déplacement du pivot (le long de l’axe caméra) par cran lors du zoom vers le curseur. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "0", UIMin = "0"))
	float RTSCursorZoomMaxPanPerEvent = 400.f;

	/** 0 = mise à jour instantanée du spring arm. >0 = interpolation (effet zoom fluide). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System", meta = (ClampMin = "0", UIMin = "0"))
	float RTSZoomInterpSpeed = 14.f;

	/** Log molette + longueurs (Output Log) — utile si le zoom « ne fait rien » (collision spring arm, etc.). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera System|Debug")
	bool bDebugLogRTSZoom = false;

public:
	virtual void Tick(float DeltaTime) override;
};
