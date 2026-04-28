// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheHouseDayNightCycleActor.generated.h"

class ADirectionalLight;
class ASkyLight;
class ASkyAtmosphere;
class AExponentialHeightFog;
class UCurveFloat;
class UCurveLinearColor;

/**
 * Cycle jour/nuit piloté par l'horloge in-game du PlayerController (durée de journée configurable).
 *
 * Usage :
 * - Place cet acteur dans la carte (ou dans un sous-niveau Lighting).
 * - Référence ton DirectionalLight (soleil), et optionnellement SkyLight / SkyAtmosphere / Fog.
 * - Les courbes sont optionnelles (si null, on applique des valeurs simples).
 */
UCLASS(Blueprintable)
class THEHOUSE_API ATheHouseDayNightCycleActor : public AActor
{
	GENERATED_BODY()

public:
	ATheHouseDayNightCycleActor();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

	/** Soleil : DirectionalLight de la scène. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	TObjectPtr<ADirectionalLight> SunLight = nullptr;

	/** SkyLight optionnel (capture dynamique). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	TObjectPtr<ASkyLight> SkyLight = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	TObjectPtr<ASkyAtmosphere> SkyAtmosphere = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	TObjectPtr<AExponentialHeightFog> ExponentialFog = nullptr;

	/** Vitesse d'application (interp). 0 = snap immédiat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0"))
	float InterpSpeed = 2.0f;

	/** Décalage de midi (0..1) : 0.5 = midi quand progress=0.5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", ClampMax="1.0"))
	float NoonOffset01 = 0.5f;

	/** Pitch du soleil à minuit / midi (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	float SunPitchAtMidnight = -90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	float SunPitchAtNoon = 55.f;

	/** Yaw du soleil (degrés) : direction générale (est/ouest). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	float SunYaw = 0.f;

	/** Intensité du soleil (lux). Si une courbe est fournie, elle remplace ce réglage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0"))
	float SunIntensityLuxAtNoon = 75000.f;

	/** Intensité du soleil en fonction du progress jour (X=0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves")
	TObjectPtr<UCurveFloat> SunIntensityLuxCurve = nullptr;

	/** Couleur lumière du soleil (X=0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves")
	TObjectPtr<UCurveLinearColor> SunLightColorCurve = nullptr;

	/** Intensité SkyLight (0..). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves")
	TObjectPtr<UCurveFloat> SkyLightIntensityCurve = nullptr;

	/** Si true : recapture du SkyLight lors des changements (peut être coûteux). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	bool bRecaptureSkyLight = true;

	/** Intervalle min entre deux recaptures (secondes IRL). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinSecondsBetweenSkyRecapture = 2.0f;

private:
	float LastSkyRecaptureRealSeconds = -1.f;

	float GetDayProgress01() const;
	void Apply(float DayProgress01, float DeltaSeconds);
};

