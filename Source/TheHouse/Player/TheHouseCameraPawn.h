// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "TheHouseCameraPawn.generated.h"

UCLASS()
class THEHOUSE_API ATheHouseCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ATheHouseCameraPawn();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Pitch RTS sur le boom (pas via le pitch du contrôleur — AddYawInput + héritage yaw seuls sur le control rotation). */
	void ApplyRTSArmPitchDelta(float DeltaDegrees);
	void SetRTSArmPitchDegrees(float PitchDegrees);
	float GetRTSArmPitchDegrees() const { return RTSArmPitchDegrees; }

	/** Camera Boom */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	USpringArmComponent* CameraBoom;

	/** RTS Camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* RTSCameraComponent;

	/** Movement Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	UFloatingPawnMovement* MovementComponent;

	/** Inclinaison actuelle du boom (degrés), clampée entre Min/Max. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|RTS", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float RTSArmPitchDegrees = -60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|RTS", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float RTSArmPitchMin = -80.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|RTS", meta = (ClampMin = "-89.0", ClampMax = "0.0"))
	float RTSArmPitchMax = -10.f;

protected:
	void SyncRTSArmPitchToBoom();

	/** Map to track actors and their current CameraFade value (0 to 1) */
	UPROPERTY(Transient)
	TMap<AActor*, float> OccludedActors;

	/** Consecutive frames without a trace hit; avoids flicker when the ray grazes or misses briefly */
	UPROPERTY(Transient)
	TMap<AActor*, int32> OcclusionMissFrames;

	/** Name of the Material Parameter to drive (Default: "CameraFade") — optional if the wall uses materials */
	FName MaterialFadeParameterName;

	/** Speed at which walls fade in/out */
	float FadeSpeed;

	/** Radius (cm) for sphere sweep along camera–pawn; reduces missed hits on thin walls */
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion")
	float OcclusionSweepRadius;

	/** Inflate bounds (cm) when testing segment vs wall box — keeps occlusion true when the sweep briefly misses */
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion")
	float OcclusionBoundsPadding;

	/** Frames without a hit before we treat the wall as no longer occluding (fade back) */
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion", meta = (ClampMin = "1", UIMin = "1"))
	int32 OcclusionMissFramesBeforeFadeOut;

	/** Debug visuel + logs pour comprendre pourquoi l'occlusion ne détecte plus */
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion|Debug")
	bool bDebugOcclusion = false;

	/** Durée des debug draws (0 = une frame) */
	UPROPERTY(EditAnywhere, Category = "Camera|Occlusion|Debug", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DebugDrawDuration = 0.0f;

	/** Checks for walls between Camera and Pawn, and updates their material parameter */
	void CheckCameraOcclusion(float DeltaTime);

	/** Segment vs expanded bounds — catches thin walls when sphere sweep misses */
	void AddWallsHitBySegmentBounds(const FVector& CameraLoc, const FVector& PawnLoc, TSet<AActor*>& InOutWallsHit);

	// Helper to apply fade to all meshes on an actor
	void ApplyFadeToActor(AActor* Actor, float Value);
};
