// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/Environment/TheHouseDayNightCycleActor.h"

#include "TheHouse/Player/TheHousePlayerController.h"
#include "Components/SceneComponent.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/Engine.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Materials/Material.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Math/RotationMatrix.h"
#include "DrawDebugHelpers.h"
#include "CollisionQueryParams.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "Logging/LogMacros.h"

DEFINE_LOG_CATEGORY_STATIC(LogTheHouseDayNight, Log, All);

ATheHouseDayNightCycleActor::ATheHouseDayNightCycleActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Ensure we always have a valid root so runtime-created components (moon disk) can attach reliably
	// even when the actor is placed as a pure C++ actor (no Blueprint-added default scene root).
	if (!GetRootComponent())
	{
		USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
		SetRootComponent(Root);
	}
}

static float TheHouse_SmoothStep01(const float Edge0, const float Edge1, const float X)
{
	if (Edge1 <= Edge0)
	{
		return X >= Edge1 ? 1.f : 0.f;
	}
	const float T = FMath::Clamp((X - Edge0) / (Edge1 - Edge0), 0.f, 1.f);
	return T * T * (3.f - 2.f * T);
}

/** BP peut laisser MoonDiskTexture vide : on pousse quand même T_MoonDisk sur le MID (sinon masque Masked souvent à 0). */
static UTexture2D* TheHouse_ResolveMoonDiskTexture(UTexture2D* ExplicitFromActor)
{
	if (ExplicitFromActor)
	{
		return ExplicitFromActor;
	}
	static UTexture2D* GDefaultMoonDiskTex = nullptr;
	static bool bLoggedMoonTexLoadFail = false;
	if (!GDefaultMoonDiskTex)
	{
		GDefaultMoonDiskTex = LoadObject<UTexture2D>(
			nullptr, TEXT("/Game/Environment/Moon/T_MoonDisk.T_MoonDisk"), nullptr, LOAD_None, nullptr);
		if (!GDefaultMoonDiskTex && !bLoggedMoonTexLoadFail)
		{
			bLoggedMoonTexLoadFail = true;
			UE_LOG(LogTheHouseDayNight, Warning,
				TEXT("Moon disk: impossible de charger /Game/Environment/Moon/T_MoonDisk — assigne Moon Disk Texture sur l’acteur ou vérifie le chemin d’asset."));
		}
	}
	return GDefaultMoonDiskTex;
}

/** BP peut laisser MoonDiskMaterial vide : on essaye un matériau par défaut M_MoonDisk. */
static UMaterialInterface* TheHouse_ResolveMoonDiskMaterial(UMaterialInterface* ExplicitFromActor)
{
	if (ExplicitFromActor)
	{
		return ExplicitFromActor;
	}
	static UMaterialInterface* GDefaultMoonDiskMat = nullptr;
	static bool bLoggedMoonMatLoadFail = false;
	if (!GDefaultMoonDiskMat)
	{
		GDefaultMoonDiskMat = LoadObject<UMaterialInterface>(
			nullptr, TEXT("/Game/Environment/Moon/M_MoonDisk.M_MoonDisk"), nullptr, LOAD_None, nullptr);
		if (!GDefaultMoonDiskMat && !bLoggedMoonMatLoadFail)
		{
			bLoggedMoonMatLoadFail = true;
			UE_LOG(LogTheHouseDayNight, Warning,
				TEXT("Moon disk: impossible de charger /Game/Environment/Moon/M_MoonDisk — assigne Moon Disk Material sur l’acteur ou vérifie le chemin d’asset."));
		}
	}
	return GDefaultMoonDiskMat;
}

void ATheHouseDayNightCycleActor::BeginPlay()
{
	Super::BeginPlay();
	LastSkyRecaptureRealSeconds = -1.f;
	BeginPlayRealSeconds = GetWorld() ? GetWorld()->GetRealTimeSeconds() : -1.f;
	bDidForceDynamicLightingOnce = false;
	bHasPrevMoonDiskCam = false;
	MoonDiskCameraStableAccumSeconds = 0.f;
	bMoonDiskCameraStabilitySatisfied = false;
	MoonDiskOcclusionVisSmoothed01 = 1.f;

	// Safety: debug flags can force a giant on-screen quad; keep them opt-in.
	// If you want to use them, enable bDebugDayNight (or toggle at runtime while debugging).
	if (!bDebugDayNight)
	{
		bDebugPinMoonDiskToCameraForward = false;
		bDebugForceMoonDiskVisible = false;
		bDebugMoonDiskPlacement = false;
	}

	EnsureMoonDiskVisual();
}

void ATheHouseDayNightCycleActor::EnsureDynamicLightingSetupOnce()
{
	if (bDidForceDynamicLightingOnce || !bForceDynamicLightingForCycle)
	{
		return;
	}
	bDidForceDynamicLightingOnce = true;

	auto ConfigureDirectional = [](ADirectionalLight* Dir, const int32 AtmosphereSunIndex, const bool bAtmosphereSun)
	{
		if (!Dir)
		{
			return;
		}
		if (UDirectionalLightComponent* Dlc = Cast<UDirectionalLightComponent>(Dir->GetLightComponent()))
		{
			Dlc->SetMobility(EComponentMobility::Movable);
			Dlc->SetCastShadows(true);
			Dlc->bAffectsWorld = true;
			// Only the sun should drive SkyAtmosphere "sun disk" / secondary sun visuals.
			// Moon as a second atmosphere sun creates a visible halo/disk where the moon light comes from.
			Dlc->bAtmosphereSunLight = bAtmosphereSun;
			Dlc->AtmosphereSunLightIndex = bAtmosphereSun ? AtmosphereSunIndex : 0;
		}
	};

	ConfigureDirectional(SunLight, 0, true);
	ConfigureDirectional(MoonLight, 1, bMoonLightAffectsSkyAtmosphere);

	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			SkyComp->SetMobility(EComponentMobility::Movable);
			SkyComp->bAffectsWorld = true;
			SkyComp->SetCastShadows(true);
			SkyComp->SetRealTimeCapture(true);
		}
	}
}

void ATheHouseDayNightCycleActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float P = GetDayProgress01();
	Apply(P, DeltaSeconds);

	if (bDebugDayNight && GEngine && GetWorld())
	{
		const FString Msg = FString::Printf(
			TEXT("DayProgress01=%.3f | SunElev=%.1fdeg SunLux=%.0f | MoonElev=%.1fdeg MoonLux=%.1f MoonVisL=%.2f MoonDiskVis=%.2f | Sun=%s | Moon=%s"),
			P,
			LastSunElevationDeg,
			LastSunLux,
			LastMoonElevationDeg,
			LastMoonLux,
			LastMoonVis01,
			LastMoonDiskVis01,
			SunLight ? *SunLight->GetName() : TEXT("None"),
			MoonLight ? *MoonLight->GetName() : TEXT("None"));
		GEngine->AddOnScreenDebugMessage((int32)((PTRINT)this & 0x7FFFFFFF), 0.f, FColor::Cyan, Msg);
	}
}

float ATheHouseDayNightCycleActor::GetDayProgress01() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.f;
	}
	// Tous les PC (split-screen / ordre indéterminé), pas seulement l’index 0 casté trop tôt.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ATheHousePlayerController* TH = Cast<ATheHousePlayerController>(It->Get()))
		{
			return FMath::Clamp(TH->GetInGameDayProgress01(), 0.f, 1.f);
		}
	}
	return 0.f;
}

void ATheHouseDayNightCycleActor::GetCameraViewForMoonDisk(APlayerController* PC, FVector& OutLoc, FRotator& OutRot)
{
	if (!PC)
	{
		OutLoc = FVector::ZeroVector;
		OutRot = FRotator::ZeroRotator;
		return;
	}
	// Prefer the camera manager: it tends to be valid earlier (first frames) than a pawn camera component.
	if (PC->PlayerCameraManager)
	{
		PC->PlayerCameraManager->GetCameraViewPoint(OutLoc, OutRot);
		if (!OutLoc.IsNearlyZero() || !OutRot.IsNearlyZero())
		{
			return;
		}
	}
	// RTS : la vue réelle est sur le pawn (SpringArm + CameraComponent). Le PCM peut diverger du socket caméra.
	if (APawn* ViewPawn = PC->GetPawn())
	{
		if (UCameraComponent* Cam = ViewPawn->FindComponentByClass<UCameraComponent>())
		{
			OutLoc = Cam->GetComponentLocation();
			OutRot = Cam->GetComponentRotation();
			return;
		}
	}
	PC->GetPlayerViewPoint(OutLoc, OutRot);
}

APlayerController* ATheHouseDayNightCycleActor::GetLocalViewingPlayerController(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}
	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		return PC;
	}
	return UGameplayStatics::GetPlayerController(World, 0);
}

static float Wrap01(const float X)
{
	float R = FMath::Fmod(X, 1.f);
	if (R < 0.f)
	{
		R += 1.f;
	}
	return R;
}

void ATheHouseDayNightCycleActor::ApplyMoonDiskMaterials()
{
	if (!MoonDiskMesh)
	{
		return;
	}

	UMaterialInterface* ParentMat = TheHouse_ResolveMoonDiskMaterial(MoonDiskMaterial.Get());
	if (!ParentMat)
	{
		ParentMat = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	// Rebuild only when the parent asset changes. (If MID creation failed once, bound==ParentMat and we keep the static parent — do not spam Create every frame.)
	if (MoonDiskMaterialBound != ParentMat)
	{
		MoonDiskMID = UMaterialInstanceDynamic::Create(ParentMat, this);
		if (MoonDiskMID)
		{
			MoonDiskMaterialBound = ParentMat;
			MoonDiskMesh->SetMaterial(0, MoonDiskMID);
		}
		else
		{
			MoonDiskMID = nullptr;
			MoonDiskMaterialBound = ParentMat;
			MoonDiskMesh->SetMaterial(0, ParentMat);
		}
	}

	if (MoonDiskMID)
	{
		if (UTexture2D* Tex = TheHouse_ResolveMoonDiskTexture(MoonDiskTexture.Get()))
		{
			MoonDiskMID->SetTextureParameterValue(FName(TEXT("MoonTex")), Tex);
		}
		else
		{
			static bool bOnce = false;
			if (!bOnce)
			{
				bOnce = true;
				UE_LOG(LogTheHouseDayNight, Warning,
					TEXT("Moon disk: aucune texture (MoonTex) — le MID n’aura pas la lune ; assigne Moon Disk Texture ou importe T_MoonDisk sous /Game/Environment/Moon/."));
			}
		}
	}
	else if (MoonDiskMaterial)
	{
		static bool bOnceMidFail = false;
		if (!bOnceMidFail)
		{
			bOnceMidFail = true;
			UE_LOG(LogTheHouseDayNight, Warning,
				TEXT("Moon disk: UMaterialInstanceDynamic::Create a échoué pour %s — vérifie le parent matériau."),
				*MoonDiskMaterial->GetName());
		}
	}
}

void ATheHouseDayNightCycleActor::EnsureMoonDiskVisual()
{
	// If disabled, ensure the runtime component is hidden (actor instances can keep old state in the level).
	if (!bEnableMoonDiskVisual)
	{
		if (MoonDiskMesh)
		{
			MoonDiskMesh->SetHiddenInGame(true, true);
			MoonDiskMesh->SetVisibility(false, true);
		}
		MoonDiskOcclusionVisSmoothed01 = 1.f;
		return;
	}

	if (!GetWorld() || !GetRootComponent())
	{
		return;
	}

	// Force a rebind when the guaranteed debug flag toggles.
	UMaterialInterface* ResolvedMaterial = TheHouse_ResolveMoonDiskMaterial(MoonDiskMaterial.Get());
	UMaterialInterface* const DesiredDiskMaterial = ResolvedMaterial
		? ResolvedMaterial
		: UMaterial::GetDefaultMaterial(MD_Surface);

	// NOTE: If ResolvedMaterial is null, we silently fall back to DefaultMaterial (checker).

	if (MoonDiskMesh && MoonDiskMaterialBound != DesiredDiskMaterial)
	{
		MoonDiskMesh->DestroyComponent(false);
		MoonDiskMesh = nullptr;
		MoonDiskMID = nullptr;
		MoonDiskMaterialBound = nullptr;
	}

	if (MoonDiskMesh)
	{
		ApplyMoonDiskMaterials();
		return;
	}

	MoonDiskMesh = NewObject<UStaticMeshComponent>(this, TEXT("MoonDiskMesh"));
	if (!MoonDiskMesh)
	{
		return;
	}

	MoonDiskMesh->RegisterComponent();
	MoonDiskMesh->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	MoonDiskMesh->SetMobility(EComponentMobility::Movable);
	MoonDiskMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MoonDiskMesh->SetGenerateOverlapEvents(false);
	MoonDiskMesh->SetCastShadow(false);
	MoonDiskMesh->SetReceivesDecals(false);
	MoonDiskMesh->bVisibleInReflectionCaptures = false;
	MoonDiskMesh->bVisibleInRayTracing = false;
	MoonDiskMesh->SetTranslucentSortPriority(100000);
	// Foreground makes the disk render above world geometry (can look like it goes through the ground).
	// Default: let the world occlude it for more realism.
	MoonDiskMesh->SetDepthPriorityGroup(
		bMoonDiskRenderInForeground ? ESceneDepthPriorityGroup::SDPG_Foreground : ESceneDepthPriorityGroup::SDPG_World);

	// Engine content path uses folders, not dot-separated packages.
	if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
	{
		MoonDiskMesh->SetStaticMesh(Plane);
	}
	else
	{
		static bool bLoggedPlaneFail = false;
		if (!bLoggedPlaneFail)
		{
			bLoggedPlaneFail = true;
			UE_LOG(LogTheHouseDayNight, Warning,
				TEXT("Moon disk: impossible de charger le mesh /Engine/BasicShapes/Plane.Plane — le disque ne pourra pas s’afficher."));
		}
	}

	ApplyMoonDiskMaterials();

	MoonDiskMesh->SetHiddenInGame(true, true);
}

void ATheHouseDayNightCycleActor::UpdateMoonDiskVisual(const FVector& MoonWorldDir, const float MoonVisibility01)
{
	if (!bEnableMoonDiskVisual)
	{
		if (MoonDiskMesh)
		{
			MoonDiskMesh->SetHiddenInGame(true, true);
		}
		return;
	}

	if (!MoonDiskMesh)
	{
		return;
	}

	ApplyMoonDiskMaterials();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Warmup: first frames often have unstable camera (spawn/possession/blend). Hide the disk briefly to avoid jumps.
	if (!bDebugPinMoonDiskToCameraForward && MoonDiskCameraWarmupSeconds > 0.f && BeginPlayRealSeconds >= 0.f)
	{
		const float Now = World->GetRealTimeSeconds();
		if ((Now - BeginPlayRealSeconds) < MoonDiskCameraWarmupSeconds)
		{
			MoonDiskMesh->SetHiddenInGame(true, true);
			MoonDiskMesh->SetVisibility(false, true);
			return;
		}
	}

	APlayerController* PC = GetLocalViewingPlayerController(World);
	if (!PC)
	{
		MoonDiskMesh->SetHiddenInGame(true, true);
		return;
	}

	FVector CamLoc;
	FRotator CamRot;
	GetCameraViewForMoonDisk(PC, CamLoc, CamRot);

	// Optional stability gate: hide the disk until the camera has been stable for some time.
	// This prevents "moon jumping around" during spawn / possession / camera blends.
	if (!bMoonDiskCameraStabilitySatisfied && !bDebugPinMoonDiskToCameraForward && MoonDiskRequireStableCameraSeconds > 0.f)
	{
		const float Dt = FMath::Max(0.f, World->GetDeltaSeconds());
		if (bHasPrevMoonDiskCam)
		{
			const float PosDelta = FVector::Dist(CamLoc, PrevMoonDiskCamLoc);
			const FRotator RotDelta = (CamRot - PrevMoonDiskCamRot).GetNormalized();
			const float RotDeltaDeg = FMath::Abs(RotDelta.Yaw) + FMath::Abs(RotDelta.Pitch) + FMath::Abs(RotDelta.Roll);

			const bool bPosOk = PosDelta <= FMath::Max(0.f, MoonDiskStableCameraPosThresholdUU);
			const bool bRotOk = RotDeltaDeg <= FMath::Max(0.f, MoonDiskStableCameraRotThresholdDeg);
			if (bPosOk && bRotOk)
			{
				MoonDiskCameraStableAccumSeconds += Dt;
			}
			else
			{
				MoonDiskCameraStableAccumSeconds = 0.f;
			}
		}
		bHasPrevMoonDiskCam = true;
		PrevMoonDiskCamLoc = CamLoc;
		PrevMoonDiskCamRot = CamRot;

		if (MoonDiskCameraStableAccumSeconds < MoonDiskRequireStableCameraSeconds)
		{
			MoonDiskMesh->SetHiddenInGame(true, true);
			MoonDiskMesh->SetVisibility(false, true);
			return;
		}
		bMoonDiskCameraStabilitySatisfied = true;
	}

	// Disk uses the same analytic direction as Apply() (not MoonLight forward: light can lag with RInterpTo).
	FVector TowardMoonInSky = MoonWorldDir.GetSafeNormal();
	if (TowardMoonInSky.IsNearlyZero() && MoonLight)
	{
		if (UDirectionalLightComponent* DLC = Cast<UDirectionalLightComponent>(MoonLight->GetLightComponent()))
		{
			TowardMoonInSky = (-DLC->GetForwardVector()).GetSafeNormal();
		}
		else
		{
			TowardMoonInSky = (-MoonLight->GetActorForwardVector()).GetSafeNormal();
		}
	}

	if (TowardMoonInSky.IsNearlyZero())
	{
		MoonDiskMesh->SetHiddenInGame(true, true);
		return;
	}

	// RTS camera often has constrained pitch; when the moon direction is behind the camera, the disk is technically "there"
	// but impossible to see. Keep the disk in the camera forward hemisphere by reflecting it across the view plane.
	const FVector CamForward = CamRot.Vector().GetSafeNormal();
	float DotBeforeHemisphereFix = 0.f;
	float DotAfterHemisphereFix = 0.f;
	if (!CamForward.IsNearlyZero())
	{
		if (bDebugPinMoonDiskToCameraForward)
		{
			TowardMoonInSky = CamForward;
			DotBeforeHemisphereFix = 1.f;
			DotAfterHemisphereFix = 1.f;
		}

		const float DotFwd = FVector::DotProduct(TowardMoonInSky, CamForward);
		DotBeforeHemisphereFix = DotFwd;
		if (DotFwd < 0.f)
		{
			// Reflect across plane with normal CamForward: v' = v - 2*(v·n)*n
			TowardMoonInSky = (TowardMoonInSky - 2.f * DotFwd * CamForward).GetSafeNormal();
		}
		DotAfterHemisphereFix = FVector::DotProduct(TowardMoonInSky, CamForward);
	}

	const float DiskDistance = bDebugPinMoonDiskToCameraForward ? 5000.f : FMath::Max(100.f, MoonDiskDistanceUU);

	// Occlusion fade (treat moon as "at infinity" for hills).
	float OcclusionVisTarget01 = 1.f;

	// In pinned debug mode, do not occlusion-test: we want a deterministic on-screen quad.
	if (bMoonDiskOcclusionTrace && !bDebugPinMoonDiskToCameraForward)
	{
		const float TraceDistance = FMath::Max(DiskDistance, FMath::Max(0.f, MoonDiskOcclusionTraceDistanceUU));
		const FVector TraceEnd = CamLoc + TowardMoonInSky * TraceDistance;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TheHouseMoonDiskOcc), false, this);
		Params.AddIgnoredActor(this);
		if (APawn* ViewPawn = PC->GetPawn())
		{
			Params.AddIgnoredActor(ViewPawn);
		}
		if (World->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params))
		{
			if (Hit.bBlockingHit)
			{
				const float HitDist = FVector::Dist(CamLoc, Hit.ImpactPoint);
				const float FadeLen = FMath::Max(0.f, MoonDiskOcclusionSoftFadeDistanceUU);
				if (FadeLen <= KINDA_SMALL_NUMBER)
				{
					OcclusionVisTarget01 = (HitDist < TraceDistance - 50.f) ? 0.f : 1.f;
				}
				else
				{
					// 0 when hit is "deep" (well before the trace end), 1 when hit is near the end (clear sky).
					OcclusionVisTarget01 = TheHouse_SmoothStep01(TraceDistance - FadeLen, TraceDistance, HitDist);
				}
			}
		}
	}

	// Temporal smoothing to avoid pops when trace starts/stops hitting.
	{
		const float Dt = FMath::Max(0.f, World->GetDeltaSeconds());
		const float FadeSeconds = FMath::Max(0.f, MoonDiskOcclusionFadeSeconds);
		if (FadeSeconds <= KINDA_SMALL_NUMBER)
		{
			MoonDiskOcclusionVisSmoothed01 = OcclusionVisTarget01;
		}
		else
		{
			const float Speed = 1.f / FadeSeconds;
			MoonDiskOcclusionVisSmoothed01 =
				FMath::FInterpTo(MoonDiskOcclusionVisSmoothed01, OcclusionVisTarget01, Dt, Speed);
		}
	}

	const FVector MoonPos = CamLoc + TowardMoonInSky * DiskDistance;
	// Plane mesh must face the camera: align local +Z (plane normal) toward the camera.
	FVector ZAxis = (CamLoc - MoonPos).GetSafeNormal();
	if (ZAxis.IsNearlyZero())
	{
		ZAxis = FVector::ForwardVector;
	}
	FVector XReference = FVector::UpVector;
	if (FMath::Abs(FVector::DotProduct(ZAxis, XReference)) > 0.95f)
	{
		XReference = FVector::RightVector;
	}
	const FRotator FaceCam = FRotationMatrix::MakeFromZX(ZAxis, XReference).Rotator();
	const FQuat BillQuat = FaceCam.Quaternion() * MoonDiskRotationAdjust.Quaternion();

	MoonDiskMesh->SetWorldLocation(MoonPos);
	MoonDiskMesh->SetWorldRotation(BillQuat.Rotator());
	MoonDiskMesh->SetWorldScale3D(FVector(MoonDiskScale));

	// If we pin to camera forward, we want a deterministic "always visible" disk for troubleshooting,
	// even if the current moon visibility logic would hide it (time of day / horizon fade).
	const float EffectiveVis01 = bDebugPinMoonDiskToCameraForward ? 1.f : (MoonVisibility01 * MoonDiskOcclusionVisSmoothed01);
	const bool bShow = bDebugPinMoonDiskToCameraForward ? true : (EffectiveVis01 > 0.001f);
	MoonDiskMesh->SetHiddenInGame(!bShow, true);
	MoonDiskMesh->SetVisibility(bShow, true);

	if (MoonDiskMID)
	{
		if (bShow)
		{
			const float VisForEmissive = bDebugPinMoonDiskToCameraForward ? 1.f : EffectiveVis01;
			const float RawEmissive = MoonDiskEmissive * VisForEmissive;
			const float EmissiveOut = FMath::Max(RawEmissive, MoonDiskEmissiveFloorWhenVisible);
			MoonDiskMID->SetScalarParameterValue(FName(TEXT("EmissiveScale")), EmissiveOut);
		}
		else
		{
			MoonDiskMID->SetScalarParameterValue(FName(TEXT("EmissiveScale")), 0.f);
		}
		if (UTexture2D* Tex = TheHouse_ResolveMoonDiskTexture(MoonDiskTexture.Get()))
		{
			MoonDiskMID->SetTextureParameterValue(FName(TEXT("MoonTex")), Tex);
		}
	}

	if (bDebugMoonDiskPlacement && World)
	{
		DrawDebugLine(World, CamLoc, MoonPos, FColor::Green, false, 0.f, 0, 2.f);
		DrawDebugSphere(World, MoonPos, FMath::Max(800.f, MoonDiskScale * 2.f), 8, FColor::Yellow, false, 0.f, 0, 1.f);

		if (GEngine)
		{
			UMaterialInterface* const Mat0 = MoonDiskMesh ? MoonDiskMesh->GetMaterial(0) : nullptr;
			UTexture2D* const ResolvedTex = TheHouse_ResolveMoonDiskTexture(MoonDiskTexture.Get());

			bool bMidHasMoonTexParam = false;
			FString MidMoonTexValueName(TEXT("None"));
			bool bMidHasEmissiveParam = false;
			float MidEmissiveValue = -1.f;
			if (MoonDiskMID)
			{
				UTexture* MidTex = nullptr;
				bMidHasMoonTexParam = MoonDiskMID->GetTextureParameterValue(FName(TEXT("MoonTex")), MidTex);
				MidMoonTexValueName = (MidTex ? MidTex->GetName() : TEXT("None"));
				bMidHasEmissiveParam = MoonDiskMID->GetScalarParameterValue(FName(TEXT("EmissiveScale")), MidEmissiveValue);
			}

			const float DotDbg = DotAfterHemisphereFix;
			const float AngleDeg = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(DotDbg, -1.f, 1.f)));
			const FString Info = FString::Printf(
				TEXT("MoonDisk bShow=%s Vis=%.3f MID=%s Mat=%s Tex=%s | pin=%s | dot(before=%.2f after=%.2f) angle=%.0fdeg | MID MoonTex param=%s val=%s | EmissiveScale param=%s val=%.1f | Hidden=%s VisComp=%s"),
				bShow ? TEXT("true") : TEXT("false"),
				MoonVisibility01,
				MoonDiskMID ? TEXT("yes") : TEXT("no"),
				Mat0 ? *Mat0->GetName() : TEXT("None"),
				ResolvedTex ? *ResolvedTex->GetName() : TEXT("None"),
				bDebugPinMoonDiskToCameraForward ? TEXT("true") : TEXT("false"),
				DotBeforeHemisphereFix,
				DotAfterHemisphereFix,
				AngleDeg,
				bMidHasMoonTexParam ? TEXT("YES") : TEXT("NO"),
				*MidMoonTexValueName,
				bMidHasEmissiveParam ? TEXT("YES") : TEXT("NO"),
				MidEmissiveValue,
				(MoonDiskMesh && MoonDiskMesh->bHiddenInGame) ? TEXT("true") : TEXT("false"),
				(MoonDiskMesh && MoonDiskMesh->IsVisible()) ? TEXT("true") : TEXT("false"));
			const FString Info2 = FString::Printf(
				TEXT("MoonDisk MID MoonTex param=%s val=%s | EmissiveScale param=%s val=%.1f"),
				bMidHasMoonTexParam ? TEXT("YES") : TEXT("NO"),
				*MidMoonTexValueName,
				bMidHasEmissiveParam ? TEXT("YES") : TEXT("NO"),
				MidEmissiveValue);
			GEngine->AddOnScreenDebugMessage((int32)((PTRINT)this ^ 0x4D4F4F4E), 0.f, FColor::Green, Info);
			GEngine->AddOnScreenDebugMessage((int32)((PTRINT)this ^ 0x4D4F4F4E ^ 0x1234), 0.f, FColor::Green, Info2);
		}
	}
}

void ATheHouseDayNightCycleActor::Apply(const float DayProgress01, const float DeltaSeconds)
{
	EnsureDynamicLightingSetupOnce();
	EnsureMoonDiskVisual();
	LastSunElevationDeg = 0.f;
	LastMoonElevationDeg = 0.f;
	LastSunLux = 0.f;
	LastMoonLux = 0.f;
	LastMoonVis01 = 0.f;
	LastMoonDiskVis01 = 0.f;

	// Map [0..1) day progress to a phase T where T=0 is midnight and T=0.5 is noon (wrap).
	// NoonOffset01 should represent the progress at which it is noon (default 0.5).
	const float Shift = (NoonOffset01 - 0.5f);
	const float T = Wrap01(DayProgress01 - Shift);

	// Natural transit: azimuth advances through the full cycle. Day length is configurable (summer/winter vibe).
	const float NoonElevationDeg = FMath::Max(0.f, -SunPitchAtNoon);
	const float MidnightDepthDeg = FMath::Max(0.f, SunPitchAtMidnight);
	const float DayFraction = FMath::Clamp(DaylightHours / 24.f, 0.25f, 0.8333f);
	const float DayHalfWindow = DayFraction * 0.5f;
	const float NightHalfWindow = FMath::Max(0.001f, 0.5f - DayHalfWindow);

	// Signed distance to solar noon in wrapped day space: 0 at noon, +/-0.5 at midnight.
	const float DistToNoon = FMath::Abs(T - 0.5f);
	const float WrapDistToNoon = FMath::Min(DistToNoon, 1.f - DistToNoon);

	float SunElevationDeg = 0.f;
	float DiurnalAlpha = 0.f;
	if (WrapDistToNoon <= DayHalfWindow)
	{
		// Day segment: 0 at sunrise/sunset -> 1 at noon.
		const float DayT = 1.f - FMath::Clamp(WrapDistToNoon / FMath::Max(0.001f, DayHalfWindow), 0.f, 1.f);
		const float DayShape = FMath::Sin(DayT * (PI * 0.5f));
		DiurnalAlpha = DayShape;
		SunElevationDeg = DayShape * NoonElevationDeg;
	}
	else
	{
		// Night segment: 0 at dusk/dawn -> 1 at midnight.
		const float NightT = FMath::Clamp((WrapDistToNoon - DayHalfWindow) / NightHalfWindow, 0.f, 1.f);
		const float NightShape = FMath::Sin(NightT * (PI * 0.5f));
		SunElevationDeg = -NightShape * MidnightDepthDeg;
	}
	const float SunAzimuthDeg = SunYaw + (T - 0.5f) * 360.f;
	const float SunElevRad = FMath::DegreesToRadians(SunElevationDeg);
	const float SunAzimuthRad = FMath::DegreesToRadians(SunAzimuthDeg);
	const float CosElev = FMath::Cos(SunElevRad);

	const FVector SunDirForBlend = FVector(
		CosElev * FMath::Cos(SunAzimuthRad),
		CosElev * FMath::Sin(SunAzimuthRad),
		FMath::Sin(SunElevRad)).GetSafeNormal();
	// DirectionalLight points in the direction rays travel (from sky to world),
	// so we orient it opposite to the celestial sun vector.
	const FRotator TargetRot = (-SunDirForBlend).Rotation();

	const float FadeDeg = FMath::Max(0.f, HorizonFadePitchDegrees);
	const float FadeRad = FMath::DegreesToRadians(FadeDeg);
	const float SunHorizonAlpha = (FadeRad <= KINDA_SMALL_NUMBER)
		? (SunElevRad > 0.f ? 1.f : 0.f)
		: TheHouse_SmoothStep01(-FadeRad, FadeRad, SunElevRad);

	const float OverlapRad = FMath::DegreesToRadians(FMath::Max(0.f, TwilightSunMoonOverlapDegrees));
	LastSunElevationDeg = SunElevationDeg;

	if (SunLight)
	{
		const FRotator Current = SunLight->GetActorRotation();
		const FRotator NewRot = (InterpSpeed <= 0.f)
			? TargetRot
			: FMath::RInterpTo(Current, TargetRot, DeltaSeconds, InterpSpeed);
		SunLight->SetActorRotation(NewRot);

		if (ULightComponent* LightComp = SunLight->GetLightComponent())
		{
			float TargetLux =
				SunIntensityLuxCurve ? FMath::Max(0.f, SunIntensityLuxCurve->GetFloatValue(DayProgress01))
									 : FMath::Lerp(0.f, SunIntensityLuxAtNoon, DiurnalAlpha);

			// Smooth fade around the horizon + optional twilight overlap with moon (avoid a black gap).
			const float SunTwilightFromMoon = TheHouse_SmoothStep01(-OverlapRad, 0.f, -SunElevRad);
			const float SunBlend = FMath::Clamp(SunHorizonAlpha * (1.f - SunTwilightFromMoon * 0.35f), 0.f, 1.f);
			TargetLux *= SunBlend;
			LastSunLux = TargetLux;

			const float NewLux = (InterpSpeed <= 0.f)
				? TargetLux
				: FMath::FInterpTo(LightComp->Intensity, TargetLux, DeltaSeconds, InterpSpeed);
			LightComp->SetIntensity(NewLux);

			if (SunLightColorCurve && SunElevRad > 0.f)
			{
				const FLinearColor Target = SunLightColorCurve->GetLinearColorValue(DayProgress01);
				const FLinearColor CurrentC = LightComp->LightColor;
				const FLinearColor NewC = (InterpSpeed <= 0.f)
					? Target
					: FMath::CInterpTo(CurrentC, Target, DeltaSeconds, InterpSpeed);
				LightComp->SetLightColor(NewC);
			}
		}
	}

	// Moon: shared celestial math for DirectionalLight + moon disk (disk must not depend only on MoonLight being valid).
	FVector MoonDirN = FVector::ZeroVector;
	float MoonVisForDisk = 0.f;

	const bool bRunMoonCelestialMath = (bEnableMoonLight && MoonLight) || bEnableMoonDiskVisual;
	if (bRunMoonCelestialMath)
	{
		const float MoonT = Wrap01(T + FMath::Clamp(MoonPhaseOffset01, 0.f, 1.f));
		const float MoonDistToNoon = FMath::Abs(MoonT - 0.5f);
		const float MoonWrapDistToNoon = FMath::Min(MoonDistToNoon, 1.f - MoonDistToNoon);
		float MoonElevationDeg = 0.f;
		if (MoonWrapDistToNoon <= DayHalfWindow)
		{
			const float MoonDayT = 1.f - FMath::Clamp(MoonWrapDistToNoon / FMath::Max(0.001f, DayHalfWindow), 0.f, 1.f);
			MoonElevationDeg = FMath::Sin(MoonDayT * (PI * 0.5f)) * NoonElevationDeg;
		}
		else
		{
			const float MoonNightT = FMath::Clamp((MoonWrapDistToNoon - DayHalfWindow) / NightHalfWindow, 0.f, 1.f);
			MoonElevationDeg = -FMath::Sin(MoonNightT * (PI * 0.5f)) * MidnightDepthDeg;
		}
		const float MoonAzimuthDeg = SunYaw + (MoonT - 0.5f) * 360.f;
		const float MoonElevRadForDir = FMath::DegreesToRadians(MoonElevationDeg);
		const float MoonAzimuthRad = FMath::DegreesToRadians(MoonAzimuthDeg);
		const float MoonCosElev = FMath::Cos(MoonElevRadForDir);
		const FVector MoonDir = FVector(
			MoonCosElev * FMath::Cos(MoonAzimuthRad),
			MoonCosElev * FMath::Sin(MoonAzimuthRad),
			FMath::Sin(MoonElevRadForDir)).GetSafeNormal();

		MoonDirN = MoonDir.GetSafeNormal();
		const float MoonElevRad = FMath::Asin(FMath::Clamp(MoonDirN.Z, -1.f, 1.f));
		LastMoonElevationDeg = FMath::RadiansToDegrees(MoonElevRad);

		const float MoonHorizonAlpha = (FadeRad <= KINDA_SMALL_NUMBER)
			? (MoonElevRad > 0.f ? 1.f : 0.f)
			: TheHouse_SmoothStep01(-FadeRad, FadeRad, MoonElevRad);

		const float MoonTwilightFromSun = (OverlapRad <= KINDA_SMALL_NUMBER)
			? 0.f
			: (1.f - TheHouse_SmoothStep01(0.f, OverlapRad, SunElevRad));

		// Light blending: smooth handoff at horizon + twilight overlap (OR-style).
		const float MoonVisForLight = FMath::Clamp(
			MoonHorizonAlpha + MoonTwilightFromSun - MoonHorizonAlpha * MoonTwilightFromSun, 0.f, 1.f);
		// Disk: horizon × atténuation soleil. Si TwilightSunMoonOverlapDegrees == 0, MoonTwilightFromSun est 0 pour la lumière
		// (OK) mais le produit disque serait toujours 0 — on traite alors « pas de washout » comme facteur 1.
		const float MoonDiskSunFade = (OverlapRad <= KINDA_SMALL_NUMBER)
			? 1.f
			: (1.f - TheHouse_SmoothStep01(0.f, OverlapRad, SunElevRad));
		MoonVisForDisk = FMath::Clamp(MoonHorizonAlpha * MoonDiskSunFade, 0.f, 1.f);

		if (bEnableMoonLight && MoonLight)
		{
			const FRotator MoonTargetRot = (-MoonDir).Rotation();

			const FRotator Current = MoonLight->GetActorRotation();
			const FRotator NewRot = (InterpSpeed <= 0.f)
				? MoonTargetRot
				: FMath::RInterpTo(Current, MoonTargetRot, DeltaSeconds, InterpSpeed);
			MoonLight->SetActorRotation(NewRot);

			if (ULightComponent* LightComp = MoonLight->GetLightComponent())
			{
				float TargetLux =
					MoonIntensityLuxCurve ? FMath::Max(0.f, MoonIntensityLuxCurve->GetFloatValue(DayProgress01))
										  : FMath::Lerp(MoonIntensityLuxAtMidnight, 0.f, DiurnalAlpha);

				const float MoonVis = MoonVisForLight;
				TargetLux = FMath::Max(TargetLux * MoonVis, MoonMinimumVisibleLux * MoonVis);
				LastMoonLux = TargetLux;
				LastMoonVis01 = MoonVis;

				const float NewLux = (InterpSpeed <= 0.f)
					? TargetLux
					: FMath::FInterpTo(LightComp->Intensity, TargetLux, DeltaSeconds, InterpSpeed);
				LightComp->SetIntensity(NewLux);

				const FLinearColor TargetC = MoonLightColorCurve
					? MoonLightColorCurve->GetLinearColorValue(DayProgress01)
					: MoonLightColorAtNight;

				const FLinearColor CurrentC = LightComp->LightColor;
				const FLinearColor NewC = (InterpSpeed <= 0.f)
					? TargetC
					: FMath::CInterpTo(CurrentC, TargetC, DeltaSeconds, InterpSpeed);
				LightComp->SetLightColor(NewC);
			}
		}
	}

	if (bEnableMoonDiskVisual)
	{
		const float Vis = bDebugForceMoonDiskVisible ? 1.f : MoonVisForDisk;
		LastMoonDiskVis01 = Vis;
		UpdateMoonDiskVisual(MoonDirN, Vis);
	}
	else
	{
		UpdateMoonDiskVisual(FVector::ZeroVector, 0.f);
	}

	// Sky light intensity + optional recapture.
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			if (SkyLightIntensityCurve)
			{
				const float Target = FMath::Max(SkyLightIntensityCurveFloor, SkyLightIntensityCurve->GetFloatValue(DayProgress01));
				const float NewI = (InterpSpeed <= 0.f)
					? Target
					: FMath::FInterpTo(SkyComp->Intensity, Target, DeltaSeconds, InterpSpeed);
				SkyComp->SetIntensity(NewI);
			}

			if (bRecaptureSkyLight)
			{
				if (const UWorld* World = GetWorld())
				{
					const float Now = World->GetRealTimeSeconds();
					if (LastSkyRecaptureRealSeconds < 0.f || (Now - LastSkyRecaptureRealSeconds) >= FMath::Max(0.f, MinSecondsBetweenSkyRecapture))
					{
						SkyComp->RecaptureSky();
						LastSkyRecaptureRealSeconds = Now;
					}
				}
			}
		}
	}
}

