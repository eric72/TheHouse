// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TheHouseLocomotionAnimInstance.generated.h"

class ACharacter;

/**
 * Anim Instance de base pour locomotion + données utiles aux Aim Offsets sur le spine.
 *
 * Assigner comme classe parente de ton Anim BP (mesh joueur FPS ou PNJ), puis dans le graphe :
 * - State machine / Blend Space 2D : utilise LocomotionSpeed + LocomotionDirectionDegrees pour marche / recul / strafe ;
 * - Booléens : bIsMoving, bIsCrouching, bIsSprinting, bIsInAir, bJustLanded, bJustEnteredAir, etc. ;
 * - Spine / tête avant le pivot complet du corps : SpineAimYawOffsetDegrees / SpineAimPitchOffsetDegrees
 *   (pertinent surtout si bDelayBodyYawDelayedMode est true sur le FPS — voir ATheHouseFPSCharacter).
 *
 * Les PNJ utilisent une heuristique de sprint (vélocité vs MaxWalkSpeed) sauf si tu branches une logique dédiée en Anim BP.
 */
UCLASS(transient, Blueprintable)
class THEHOUSE_API UTheHouseLocomotionAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** Vitesse horizontale (uu/s). Pour Blend Space 1D « Speed » ou axe d’un BS2D. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion")
	float LocomotionSpeed = 0.f;

	/**
	 * Angle du déplacement dans le repère du personnage (-180…180°, typique UE Blend Space Direction).
	 * 0 = avant, ±90 = strafe, ±180 = reculer.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion")
	float LocomotionDirectionDegrees = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion")
	bool bIsMoving = false;

	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion")
	bool bIsCrouching = false;

	/** FPS : sprint bouton ; PNJ : heuristique vitesse / MaxWalkSpeed (réglable en BP si besoin). */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion")
	bool bIsSprinting = false;

	/** MOVE_Falling (pas sur le sol navigable). Équivalent courant « isInAir / isFalling » pour l’Anim Graph. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	bool bIsInAir = false;

	/** True sur le sol avec contact (marche / idle au sol), exclut chute / nage / vol. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	bool bIsMovingOnGround = false;

	/** Front montant : était en l’air la frame d’anim précédente, au sol maintenant — transition vers anim d’atterrissage. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	bool bJustLanded = false;

	/** Front montant : au sol puis en l’air — saut, marche du bord, etc. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	bool bJustEnteredAir = false;

	/** Vitesse de chute actuelle (uu/s, positive = descend). 0 si au sol. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	float FallVerticalSpeed = 0.f;

	/**
	 * Au frame d’atterrissage : intensité max de chute sur cette chute (uu/s vers le bas).
	 * Pour choisir light / heavy landing ; retombe à 0 après la frame (copie dans une variable BP si tu veux la garder).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	float LandImpactVerticalSpeed = 0.f;

	/** Temps cumulé en chute cette session (remis à 0 au sol). Utile pour longues chutes. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|Air")
	float TimeFallingSeconds = 0.f;

	/** True si le pawn FPS a le mode « corps en retard » (voir ATheHouseFPSCharacter::bDelayBodyYawUntilLookSideways). Toujours false pour PNJ. */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|AimOffset")
	bool bBodyYawDelayedMode = false;

	/**
	 * Écart yaw contrôleur vs corps — à brancher sur un Aim Offset horizontal sur spine / chest / head
	 * pour tourner un peu le buste avant le grand pivot code.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|AimOffset")
	float SpineAimYawOffsetDegrees = 0.f;

	/** Écart pitch — Aim Offset vertical (regard haut/bas). */
	UPROPERTY(BlueprintReadOnly, Category = "TheHouse|Locomotion|AimOffset")
	float SpineAimPitchOffsetDegrees = 0.f;

	/** Seuil minimal de vitesse pour considérer « en mouvement » (idle vs locomotion). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|Locomotion|Settings")
	float MovingSpeedThreshold = 3.f;

	/** Lissage des valeurs envoyées au Blend Space (évite « pop » / jitter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|Locomotion|Settings")
	bool bSmoothLocomotionValues = true;

	/** Vitesse d'interp pour la vitesse (plus grand = suit plus vite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|Locomotion|Settings", meta=(ClampMin="0.0"))
	float LocomotionSpeedInterpRate = 12.f;

	/** Vitesse d'interp pour la direction (deg/s, plus grand = suit plus vite). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|Locomotion|Settings", meta=(ClampMin="0.0"))
	float LocomotionDirectionInterpRate = 14.f;

	/** PNJ : fraction de MaxWalkSpeed au-dessus de laquelle on considère « sprint » pour l’anim (si pas FPS). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|Locomotion|Settings", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float NPCSprintSpeedRatioThreshold = 0.72f;

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ACharacter> CachedCharacter;

	/** État frame précédente (tick anim). */
	bool bPrevAnimTickInAir = false;

	/** Pic de vitesse descendante pendant la chute en cours (uu/s). */
	float PeakDownwardSpeedWhileFalling = 0.f;

	/** Valeurs brutes calculées puis lissées. */
	float RawLocomotionSpeed = 0.f;
	float RawLocomotionDirectionDegrees = 0.f;
};
