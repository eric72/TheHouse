// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/Environment/TheHouseDayNightCycleActor.h"

#include "TheHouse/Player/TheHousePlayerController.h"
#include "Components/LightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Curves/CurveFloat.h"
#include "Curves/CurveLinearColor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Kismet/GameplayStatics.h"

ATheHouseDayNightCycleActor::ATheHouseDayNightCycleActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void ATheHouseDayNightCycleActor::BeginPlay()
{
	Super::BeginPlay();
	LastSkyRecaptureRealSeconds = -1.f;
}

void ATheHouseDayNightCycleActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Apply(GetDayProgress01(), DeltaSeconds);
}

float ATheHouseDayNightCycleActor::GetDayProgress01() const
{
	if (const UWorld* World = GetWorld())
	{
		if (ATheHousePlayerController* PC = Cast<ATheHousePlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
		{
			return FMath::Clamp(PC->GetInGameDayProgress01(), 0.f, 1.f);
		}
	}
	return 0.f;
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

void ATheHouseDayNightCycleActor::Apply(const float DayProgress01, const float DeltaSeconds)
{
	// Map [0..1) to a cosine day curve: t=0 midnight, t=0.5 noon.
	// NoonOffset01 should represent the progress at which it is noon (default 0.5).
	const float Shift = (NoonOffset01 - 0.5f);
	const float T = Wrap01(DayProgress01 - Shift);

	const float CosT = FMath::Cos(T * 2.f * PI); // 1 at midnight, -1 at noon
	const float DayAlpha = FMath::Clamp((1.f - CosT) * 0.5f, 0.f, 1.f); // 0 midnight -> 1 noon

	// Sun rotation.
	if (SunLight)
	{
		const float TargetPitch = FMath::Lerp(SunPitchAtMidnight, SunPitchAtNoon, DayAlpha);
		const bool bSunAboveHorizon = TargetPitch > 0.f;
		const FRotator TargetRot(TargetPitch, SunYaw, 0.f);
		const FRotator Current = SunLight->GetActorRotation();
		const FRotator NewRot = (InterpSpeed <= 0.f)
			? TargetRot
			: FMath::RInterpTo(Current, TargetRot, DeltaSeconds, InterpSpeed);
		SunLight->SetActorRotation(NewRot);

		if (ULightComponent* LightComp = SunLight->GetLightComponent())
		{
			float TargetLux =
				SunIntensityLuxCurve ? FMath::Max(0.f, SunIntensityLuxCurve->GetFloatValue(DayProgress01))
									 : FMath::Lerp(0.f, SunIntensityLuxAtNoon, DayAlpha);

			// Hard safety: never emit sun light at night (prevents “flashbang” when curves/exposure are misconfigured).
			if (!bSunAboveHorizon)
			{
				TargetLux = 0.f;
			}

			const float NewLux = (InterpSpeed <= 0.f)
				? TargetLux
				: FMath::FInterpTo(LightComp->Intensity, TargetLux, DeltaSeconds, InterpSpeed);
			LightComp->SetIntensity(NewLux);

			if (SunLightColorCurve && bSunAboveHorizon)
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

	// Sky light intensity + optional recapture.
	if (SkyLight)
	{
		if (USkyLightComponent* SkyComp = SkyLight->GetLightComponent())
		{
			if (SkyLightIntensityCurve)
			{
				const float Target = FMath::Max(0.f, SkyLightIntensityCurve->GetFloatValue(DayProgress01));
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

