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
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UTexture2D;
class UMaterialInterface;

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

	/** Lune optionnelle : second DirectionalLight (ex: lumière bleutée de nuit). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	TObjectPtr<ADirectionalLight> MoonLight = nullptr;

	/** Active la lune si une référence MoonLight est fournie. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	bool bEnableMoonLight = true;

	/**
	 * Si true, la DirectionalLight de la lune est enregistrée comme "Atmosphere Sun Light" (index 1).
	 * Cela permet au SkyAtmosphere / SkyLight d'avoir une teinte nocturne (bleu foncé) plus crédible.
	 * Désactive si tu vois un halo/disque atmosphérique gênant.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(EditCondition="bEnableMoonLight"))
	bool bMoonLightAffectsSkyAtmosphere = true;

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
	float SunPitchAtMidnight = 90.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	float SunPitchAtNoon = -55.f;

	/** Durée du jour en heures in-game (lever -> coucher). 12 = équinoxe, >12 = vibe été, <12 = vibe hiver. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="6.0", ClampMax="20.0", UIMin="6.0", UIMax="20.0"))
	float DaylightHours = 12.f;

	/** Largeur du fondu autour de l’horizon (degrés). Évite les “sauts” (flashbang) au lever/coucher. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0"))
	float HorizonFadePitchDegrees = 5.f;

	/**
	 * Chevauchement soleil/lune autour de l’horizon (degrés d’élévation).
	 * Plus c’est grand, plus la lune peut rester visible un peu après le lever / avant le coucher (effet “été”).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonLight"))
	float TwilightSunMoonOverlapDegrees = 8.f;

	/** Yaw du soleil (degrés) à midi. Lever = SunYaw - 90, coucher = SunYaw + 90. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	float SunYaw = 0.f;

	/**
	 * Décalage de phase orbitale de la lune (0..1). 0.5 = exactement opposée au soleil.
	 * Augmenter légèrement (>0.5) la fait apparaître un peu avant le coucher complet du soleil.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bEnableMoonLight"))
	float MoonPhaseOffset01 = 0.58f;

	/** Intensité du soleil (lux). Si une courbe est fournie, elle remplace ce réglage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0"))
	float SunIntensityLuxAtNoon = 75000.f;

	/** Intensité de la lune à minuit (lux). Si une courbe est fournie, elle remplace ce réglage. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonLight"))
	float MoonIntensityLuxAtMidnight = 2.0f;

	/**
	 * Garde un minimum d'éclairage lunaire utile quand la lune est visible (évite "nuit noire totale"
	 * si MoonIntensityLuxAtMidnight est trop bas pour l'exposition du projet).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonLight"))
	float MoonMinimumVisibleLux = 0.f;

	/** Intensité du soleil en fonction du progress jour (X=0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves")
	TObjectPtr<UCurveFloat> SunIntensityLuxCurve = nullptr;

	/** Couleur lumière du soleil (X=0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves")
	TObjectPtr<UCurveLinearColor> SunLightColorCurve = nullptr;

	/** Intensité de la lune en fonction du progress jour (X=0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves", meta=(EditCondition="bEnableMoonLight"))
	TObjectPtr<UCurveFloat> MoonIntensityLuxCurve = nullptr;

	/** Couleur lumière de la lune (X=0..1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves", meta=(EditCondition="bEnableMoonLight"))
	TObjectPtr<UCurveLinearColor> MoonLightColorCurve = nullptr;

	/** Couleur fallback de la lune si aucune courbe n'est fournie. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(EditCondition="bEnableMoonLight"))
	FLinearColor MoonLightColorAtNight = FLinearColor(0.55f, 0.65f, 1.0f, 1.0f);

	/** Affiche un disque lunaire (billboard mesh) orienté vers la caméra du joueur 0. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual")
	bool bEnableMoonDiskVisual = false;

	/** Distance (uu) du disque lunaire devant la caméra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(ClampMin="100.0", UIMin="100.0", EditCondition="bEnableMoonDiskVisual"))
	float MoonDiskDistanceUU = 250000.f;

	/** Délai (secondes IRL) après BeginPlay avant d’afficher le disque (évite les sauts pendant le blend/possession caméra). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual"))
	float MoonDiskCameraWarmupSeconds = 0.2f;

	/**
	 * Option plus robuste que le warmup: le disque reste caché tant que la caméra n'est pas stable
	 * (utile si ton spawn fait un blend plus long que 0.2s).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual"))
	float MoonDiskRequireStableCameraSeconds = 0.25f;

	/** Seuil de stabilité position (uu). Si la caméra bouge plus, on considère qu'elle n'est pas stable. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual && MoonDiskRequireStableCameraSeconds > 0.0"))
	float MoonDiskStableCameraPosThresholdUU = 30.f;

	/** Seuil de stabilité rotation (degrés). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual && MoonDiskRequireStableCameraSeconds > 0.0"))
	float MoonDiskStableCameraRotThresholdDeg = 0.5f;

	/** Échelle du mesh (Plane). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(ClampMin="0.01", UIMin="0.01", EditCondition="bEnableMoonDiskVisual"))
	float MoonDiskScale = 1200.f;

	/** Intensité d’émissif du disque (0..). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual"))
	float MoonDiskEmissive = 6.f;

	/**
	 * Plancher d’émissif quand le disque est affiché (après multiplication par MoonVis).
	 * Sans ça, MoonVis petit → EmissiveScale quasi nul → disque “invisible” même si le mesh est là.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual"))
	float MoonDiskEmissiveFloorWhenVisible = 15.f;

	/** Matériau simple attendu avec param scalar `EmissiveScale` et texture param `MoonTex` (optionnel). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(EditCondition="bEnableMoonDiskVisual"))
	TObjectPtr<UMaterialInterface> MoonDiskMaterial = nullptr;

	/** Si null, le code assigne `/Game/Environment/Moon/T_MoonDisk`. Matériau Masked : l’alpha doit découper le disque (sinon invisible). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(EditCondition="bEnableMoonDiskVisual"))
	TObjectPtr<UTexture2D> MoonDiskTexture = nullptr;

	/** Rotation appliquée après alignement caméra (BasicShapes Plane : défaut 180° yaw si la face visible est à l’envers). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(EditCondition="bEnableMoonDiskVisual"))
	FRotator MoonDiskRotationAdjust = FRotator(0.f, 180.f, 0.f);

	/** Si true, masque le disque si un sol/mur bloque la ligne caméra → position du disque. Désactivé par défaut (caméra RTS: la trace touche souvent le sol avant le “ciel”). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(EditCondition="bEnableMoonDiskVisual"))
	bool bMoonDiskOcclusionTrace = false;

	/**
	 * Distance de trace d'occlusion (uu). Utile quand `MoonDiskDistanceUU` est petit pour garder une lune "réaliste"
	 * (taille angulaire) tout en permettant aux collines lointaines d’occulter le disque.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual && bMoonDiskOcclusionTrace"))
	float MoonDiskOcclusionTraceDistanceUU = 5000000.f;

	/**
	 * Fondu doux (uu) appliqué quand l'occlusion trace touche le terrain près de l’horizon.
	 * 0 = pop instantané (ancien comportement).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual && bMoonDiskOcclusionTrace"))
	float MoonDiskOcclusionSoftFadeDistanceUU = 250000.f;

	/** Fondu doux (secondes) quand l’occlusion change (évite les pops quand la trace commence/arrête de toucher). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual",
		meta=(ClampMin="0.0", UIMin="0.0", EditCondition="bEnableMoonDiskVisual && bMoonDiskOcclusionTrace"))
	float MoonDiskOcclusionFadeSeconds = 0.35f;

	/**
	 * Si true, rend le disque en Foreground (au-dessus du monde).
	 * Par défaut false pour que le sol/les murs puissent occulter la lune (plus réaliste).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|MoonVisual", meta=(EditCondition="bEnableMoonDiskVisual"))
	bool bMoonDiskRenderInForeground = false;

	/** Intensité SkyLight (0..). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves")
	TObjectPtr<UCurveFloat> SkyLightIntensityCurve = nullptr;

	/** Plancher d'intensité SkyLight quand une courbe est utilisée (évite nuit = noir total si la courbe descend trop bas). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Curves", meta=(ClampMin="0.0", UIMin="0.0"))
	float SkyLightIntensityCurveFloor = 0.05f;

	/** Si true : recapture du SkyLight lors des changements (peut être coûteux). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	bool bRecaptureSkyLight = true;

	/**
	 * Si true : force Sun/Moon/SkyLight en Movable + flags utiles pour SkyAtmosphere (évite le cas où la rotation change
	 * mais l'éclairage reste "baked" / noir en dynamique).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight")
	bool bForceDynamicLightingForCycle = true;

	/** Intervalle min entre deux recaptures (secondes IRL). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight", meta=(ClampMin="0.0", UIMin="0.0"))
	float MinSecondsBetweenSkyRecapture = 2.0f;

	/** Debug: affiche la progression du jour et intensités. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Debug")
	bool bDebugDayNight = false;

	/** Debug: ligne/sphère autour du placement du disque lune (désactivé par défaut — évite le “carré vert” si confondu avec le mesh). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Debug")
	bool bDebugMoonDiskPlacement = false;

	/** Debug: force l’affichage du disque lune (MoonVis traité comme 1) pour valider matériau / mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Debug")
	bool bDebugForceMoonDiskVisible = false;

	/** Debug: place le disque droit devant la caméra (ignore la direction lune) pour isoler un problème de rendu. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Lighting|DayNight|Debug")
	bool bDebugPinMoonDiskToCameraForward = false;

private:
	float LastSkyRecaptureRealSeconds = -1.f;
	float BeginPlayRealSeconds = -1.f;
	bool bDidForceDynamicLightingOnce = false;

	// Camera stability gate for moon disk (runtime-only)
	bool bHasPrevMoonDiskCam = false;
	FVector PrevMoonDiskCamLoc = FVector::ZeroVector;
	FRotator PrevMoonDiskCamRot = FRotator::ZeroRotator;
	float MoonDiskCameraStableAccumSeconds = 0.f;
	bool bMoonDiskCameraStabilitySatisfied = false;

	// Occlusion fade state (runtime-only)
	float MoonDiskOcclusionVisSmoothed01 = 1.f;

	// Debug telemetry (computed in Apply)
	float LastSunElevationDeg = 0.f;
	float LastMoonElevationDeg = 0.f;
	float LastSunLux = 0.f;
	float LastMoonLux = 0.f;
	float LastMoonVis01 = 0.f;
	/** Visibilité effective du disque (produit horizon × atténuation soleil) — utile au debug. */
	float LastMoonDiskVis01 = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> MoonDiskMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MoonDiskMID = nullptr;

	/** Material used when the MID was created (detect BP changes at runtime / PIE). */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MoonDiskMaterialBound = nullptr;

	float GetDayProgress01() const;
	void Apply(float DayProgress01, float DeltaSeconds);
	void EnsureDynamicLightingSetupOnce();
	void EnsureMoonDiskVisual();
	void UpdateMoonDiskVisual(const FVector& MoonWorldDir, float MoonVisibility01);
	static void GetCameraViewForMoonDisk(APlayerController* PC, FVector& OutLoc, FRotator& OutRot);
	static APlayerController* GetLocalViewingPlayerController(UWorld* World);
	void ApplyMoonDiskMaterials();
};

