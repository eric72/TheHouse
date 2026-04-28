// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/Animation/TheHouseLocomotionAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Kismet/KismetMathLibrary.h"
#include "TheHouse/Player/TheHouseFPSCharacter.h"

namespace TheHouseAnimInternal
{
	static float CalculateDirectionDegreesXY(const FVector& VelocityWorld, const FRotator& ActorRotation)
	{
		const FVector Vel2D(VelocityWorld.X, VelocityWorld.Y, 0.f);
		const float Sq = Vel2D.SizeSquared();
		if (Sq < KINDA_SMALL_NUMBER)
		{
			return 0.f;
		}
		const FVector Dir = Vel2D * FMath::InvSqrt(Sq);
		const FVector Forward = FRotationMatrix(ActorRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(ActorRotation).GetUnitAxis(EAxis::Y);
		const float ForwardDot = FVector::DotProduct(Dir, Forward);
		const float RightDot = FVector::DotProduct(Dir, Right);
		return FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));
	}
} // namespace TheHouseAnimInternal

void UTheHouseLocomotionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	CachedCharacter.Reset();
	bPrevAnimTickInAir = false;
	PeakDownwardSpeedWhileFalling = 0.f;
	if (APawn* Pawn = TryGetPawnOwner())
	{
		CachedCharacter = Cast<ACharacter>(Pawn);
		if (ACharacter* Char = CachedCharacter.Get())
		{
			if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
			{
				bPrevAnimTickInAir = Move->IsFalling();
			}
		}
	}
}

void UTheHouseLocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	ACharacter* Char = CachedCharacter.IsValid() ? CachedCharacter.Get() : Cast<ACharacter>(TryGetPawnOwner());
	if (!Char)
	{
		bIsInAir = false;
		bIsMovingOnGround = false;
		bJustLanded = false;
		bJustEnteredAir = false;
		FallVerticalSpeed = 0.f;
		LandImpactVerticalSpeed = 0.f;
		TimeFallingSeconds = 0.f;
		return;
	}
	CachedCharacter = Char;

	UCharacterMovementComponent* Move = Char->GetCharacterMovement();
	const FVector Velocity = Move ? Move->Velocity : FVector::ZeroVector;

	const bool bNowInAir = Move && Move->IsFalling();
	bIsInAir = bNowInAir;
	bIsMovingOnGround = Move && Move->IsMovingOnGround();

	bJustEnteredAir = !bPrevAnimTickInAir && bNowInAir;
	bJustLanded = bPrevAnimTickInAir && !bNowInAir;

	if (bNowInAir)
	{
		FallVerticalSpeed = FMath::Max(0.f, -Velocity.Z);
		if (Velocity.Z < 0.f)
		{
			PeakDownwardSpeedWhileFalling = FMath::Max(PeakDownwardSpeedWhileFalling, -Velocity.Z);
		}
		TimeFallingSeconds += DeltaSeconds;
	}
	else
	{
		FallVerticalSpeed = 0.f;
		TimeFallingSeconds = 0.f;
	}

	LandImpactVerticalSpeed = 0.f;
	if (bJustLanded)
	{
		LandImpactVerticalSpeed = PeakDownwardSpeedWhileFalling;
		PeakDownwardSpeedWhileFalling = 0.f;
	}
	else if (bJustEnteredAir)
	{
		PeakDownwardSpeedWhileFalling = 0.f;
	}

	bPrevAnimTickInAir = bNowInAir;

	RawLocomotionSpeed = FVector(Velocity.X, Velocity.Y, 0.f).Size();
	const FRotator ActorRot = Char->GetActorRotation();
	FRotator DirectionBasisRot = ActorRot;
	// FPS : direction de locomotion dans le repère de la visée (yaw contrôleur),
	// sinon tourner la caméra sans tourner le corps ne change pas la "Direction".
	if (Cast<ATheHouseFPSCharacter>(Char))
	{
		if (AController* Ctrl = Char->GetController())
		{
			DirectionBasisRot = FRotator(0.f, Ctrl->GetControlRotation().Yaw, 0.f);
		}
	}
	RawLocomotionDirectionDegrees = TheHouseAnimInternal::CalculateDirectionDegreesXY(Velocity, DirectionBasisRot);

	if (bSmoothLocomotionValues)
	{
		const float SpeedRate = FMath::Max(0.f, LocomotionSpeedInterpRate);
		LocomotionSpeed = (SpeedRate <= KINDA_SMALL_NUMBER)
			? RawLocomotionSpeed
			: FMath::FInterpTo(LocomotionSpeed, RawLocomotionSpeed, DeltaSeconds, SpeedRate);

		const float DirRate = FMath::Max(0.f, LocomotionDirectionInterpRate);
		if (DirRate <= KINDA_SMALL_NUMBER)
		{
			LocomotionDirectionDegrees = RawLocomotionDirectionDegrees;
		}
		else
		{
			const float Cur = LocomotionDirectionDegrees;
			const float DeltaDir = FMath::FindDeltaAngleDegrees(Cur, RawLocomotionDirectionDegrees);
			const float Step = FMath::Clamp(DeltaDir, -DirRate * DeltaSeconds, DirRate * DeltaSeconds);
			LocomotionDirectionDegrees = FMath::UnwindDegrees(Cur + Step);
		}
	}
	else
	{
		LocomotionSpeed = RawLocomotionSpeed;
		LocomotionDirectionDegrees = RawLocomotionDirectionDegrees;
	}

	bIsMoving = LocomotionSpeed > MovingSpeedThreshold;
	bIsCrouching = Char->IsCrouched();

	bBodyYawDelayedMode = false;
	if (ATheHouseFPSCharacter* Fps = Cast<ATheHouseFPSCharacter>(Char))
	{
		bIsSprinting = Fps->IsFPSMovementSprinting();
		bBodyYawDelayedMode = Fps->bDelayBodyYawUntilLookSideways;
	}
	else
	{
		bIsSprinting = false;
		if (Move && Move->MaxWalkSpeed > KINDA_SMALL_NUMBER)
		{
			bIsSprinting = LocomotionSpeed >= Move->MaxWalkSpeed * FMath::Clamp(NPCSprintSpeedRatioThreshold, 0.1f, 1.f);
		}
	}

	if (AController* Ctrl = Char->GetController())
	{
		const FRotator ControlRot = Ctrl->GetControlRotation();
		const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(ControlRot, ActorRot);
		SpineAimYawOffsetDegrees = Delta.Yaw;
		SpineAimPitchOffsetDegrees = Delta.Pitch;
	}
	else
	{
		SpineAimYawOffsetDegrees = 0.f;
		SpineAimPitchOffsetDegrees = 0.f;
	}
}
