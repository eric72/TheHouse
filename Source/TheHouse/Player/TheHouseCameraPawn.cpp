// Fill out your copyright notice in the Description page of Project Settings.


#include "TheHouseCameraPawn.h"
#include "Components/InputComponent.h"
#include "TheHouseSmartWall.h"
#include "CollisionShape.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"

// Sets default values
ATheHouseCameraPawn::ATheHouseCameraPawn()
{
	OverrideInputComponentClass = UInputComponent::StaticClass();

 	// Set this pawn to call Tick() every frame.
	PrimaryActorTick.bCanEverTick = true;

	// No collision for the root of the camera pawn
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(Root);

	// Create a camera boom
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	// Pitch RTS : relatif sur le boom, pas le pitch du contrôleur (le yaw vient de AddYawInput + bInheritYaw).
	CameraBoom->SetRelativeRotation(FRotator(RTSArmPitchDegrees, 0.f, 0.f));
	CameraBoom->TargetArmLength = 1000.0f;
	// RTS : la collision du spring arm raccourcit souvent la caméra par rapport à TargetArmLength (gros zoom sans effet visuel).
	// Réactive dans l’éditeur / BP si tu veux éviter de traverser les murs.
	CameraBoom->bDoCollisionTest = false;
	
	// --- Smoothing / Lag Settings ---
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 10.0f; // Higher = faster catchup (less lag)
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraRotationLagSpeed = 20.0f;

	// Rotation : yaw depuis le contrôleur ; pitch stocké sur le boom (ApplyRTSArmPitchDelta).
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = true;
	CameraBoom->bInheritRoll = false;
	
	// Create a camera
	RTSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("RTSCamera"));
	RTSCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	// Movement
	MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("MovementComponent"));
	MovementComponent->MaxSpeed = 2000.f;
	MovementComponent->Acceleration = 2000.f; // Smooth acceleration
	MovementComponent->Deceleration = 2000.f; // Smooth stop
	
	// Disable actor collision
	SetActorEnableCollision(false);

	// Occlusion Defaults
	MaterialFadeParameterName = FName("CameraFade");
	FadeSpeed = 5.0f;
	OcclusionSweepRadius = 36.f;
	OcclusionBoundsPadding = 48.f;
	OcclusionMissFramesBeforeFadeOut = 14;
}

// Called when the game starts or when spawned
void ATheHouseCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	if (CameraBoom)
	{
		CameraBoom->bUsePawnControlRotation = true;
		CameraBoom->bInheritPitch = false;
		CameraBoom->bInheritYaw = true;
		CameraBoom->bInheritRoll = false;

		const float BPitch = CameraBoom->GetRelativeRotation().Pitch;
		if (FMath::IsFinite(BPitch))
		{
			RTSArmPitchDegrees = FMath::Clamp(BPitch, RTSArmPitchMin, RTSArmPitchMax);
		}
		SyncRTSArmPitchToBoom();
	}
}

void ATheHouseCameraPawn::ApplyRTSArmPitchDelta(float DeltaDegrees)
{
	RTSArmPitchDegrees = FMath::Clamp(RTSArmPitchDegrees + DeltaDegrees, RTSArmPitchMin, RTSArmPitchMax);
	SyncRTSArmPitchToBoom();
}

void ATheHouseCameraPawn::SetRTSArmPitchDegrees(float PitchDegrees)
{
	RTSArmPitchDegrees = FMath::Clamp(PitchDegrees, RTSArmPitchMin, RTSArmPitchMax);
	SyncRTSArmPitchToBoom();
}

void ATheHouseCameraPawn::SyncRTSArmPitchToBoom()
{
	if (!CameraBoom)
	{
		return;
	}
	FRotator Rel = CameraBoom->GetRelativeRotation();
	Rel.Pitch = RTSArmPitchDegrees;
	CameraBoom->SetRelativeRotation(Rel);
}

// Called every frame
void ATheHouseCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	CheckCameraOcclusion(DeltaTime);
}

// Called to bind functionality to input
void ATheHouseCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATheHouseCameraPawn::CheckCameraOcclusion(float DeltaTime)
{
	if (!RTSCameraComponent) return;

	// Important: if the SpringArm is doing collision tests, the *actual* camera position gets pushed
	// forward when a wall is between pawn and camera. If we trace from that collided camera position,
	// the wall can "disappear" from the segment after 1 frame. Use the unfixed (desired) camera pos.
	FVector CameraLoc = RTSCameraComponent->GetComponentLocation();
	if (CameraBoom)
	{
		CameraLoc = CameraBoom->GetUnfixedCameraPosition();
	}
	FVector PawnLoc = GetActorLocation(); // Ground pivot

	if (bDebugOcclusion)
	{
		DrawDebugLine(GetWorld(), CameraLoc, PawnLoc, FColor::Cyan, false, DebugDrawDuration, 0, 1.5f);
		DrawDebugSphere(GetWorld(), CameraLoc, OcclusionSweepRadius, 16, FColor::Cyan, false, DebugDrawDuration, 0, 1.0f);
		DrawDebugSphere(GetWorld(), PawnLoc, OcclusionSweepRadius, 16, FColor::Cyan, false, DebugDrawDuration, 0, 1.0f);
	}

	// 1. Sphere sweep along the segment (more stable than a zero-thickness line on thin walls / grazing angles)
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this); // Ignore camera pawn
	QueryParams.bTraceComplex = false;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

	const FCollisionShape SweepShape = FCollisionShape::MakeSphere(OcclusionSweepRadius);
	GetWorld()->SweepMultiByObjectType(
		HitResults,
		CameraLoc,
		PawnLoc,
		FQuat::Identity,
		ObjectParams,
		SweepShape,
		QueryParams);

	if (bDebugOcclusion)
	{
		int32 TaggedWalls = 0;
		for (const FHitResult& Hit : HitResults)
		{
			if (AActor* A = Hit.GetActor())
			{
				TaggedWalls += A->ActorHasTag(FName("SeeThroughWall")) ? 1 : 0;
				DrawDebugPoint(GetWorld(), Hit.ImpactPoint, 10.f, A->ActorHasTag(FName("SeeThroughWall")) ? FColor::Green : FColor::Red, false, DebugDrawDuration);
			}
		}
		UE_LOG(LogTemp, Warning, TEXT("[Occlusion] Hits=%d TaggedWalls=%d Cam=%s Pawn=%s Radius=%.1f"),
			HitResults.Num(), TaggedWalls, *CameraLoc.ToCompactString(), *PawnLoc.ToCompactString(), OcclusionSweepRadius);
	}

	// 2. Identify Walls currently hit
	TSet<AActor*> WallsHitThisFrame;

	for (const FHitResult& Hit : HitResults)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->ActorHasTag("SeeThroughWall"))
		{
			WallsHitThisFrame.Add(HitActor);
			
			// If first time seeing this wall, ensure it's in the Map
			if (!OccludedActors.Contains(HitActor))
			{
				OccludedActors.Add(HitActor, 0.0f); // Start fully visible
			}
		}
	}

	// 2b. Segment vs expanded world bounds (stable when sweep misses thin geometry while still between cam and pawn)
	AddWallsHitBySegmentBounds(CameraLoc, PawnLoc, WallsHitThisFrame);

	// 3. Process All Tracked Occluded Actors
	TArray<AActor*> ActorsToRemove;
	
	for (auto& Elem : OccludedActors)
	{
		AActor* Actor = Elem.Key;
		float& CurrentFade = Elem.Value;

		if (!IsValid(Actor)) 
		{
			ActorsToRemove.Add(Actor);
			continue;
		}

		const bool bHitThisFrame = WallsHitThisFrame.Contains(Actor);
		if (bHitThisFrame)
		{
			OcclusionMissFrames.FindOrAdd(Actor) = 0;
		}
		else
		{
			int32& MissCount = OcclusionMissFrames.FindOrAdd(Actor);
			++MissCount;
		}

		// Keep fading while occluded, or briefly after a missed trace (hysteresis)
		const int32 MissCount = OcclusionMissFrames.FindRef(Actor);
		const bool bShouldStayFaded = bHitThisFrame || (MissCount < OcclusionMissFramesBeforeFadeOut);
		const float TargetFade = bShouldStayFaded ? 1.0f : 0.0f;

		// Slightly slower return to opaque avoids popping when the sweep flickers
		const float FadeOutSpeed = FadeSpeed * 0.55f;
		const float UseSpeed = (TargetFade > CurrentFade) ? FadeSpeed : FadeOutSpeed;
		CurrentFade = FMath::FInterpTo(CurrentFade, TargetFade, DeltaTime, UseSpeed);

		// Apply to Material
		ApplyFadeToActor(Actor, CurrentFade);

		// Cleanup if fully visible and not hit
		if (TargetFade == 0.0f && FMath::IsNearlyZero(CurrentFade, 0.01f))
		{
			ActorsToRemove.Add(Actor);
			ApplyFadeToActor(Actor, 0.0f); 
		}
	}

	// 4. Remove finished actors from map
	for (AActor* Actor : ActorsToRemove)
	{
		OccludedActors.Remove(Actor);
		OcclusionMissFrames.Remove(Actor);
	}
}

void ATheHouseCameraPawn::AddWallsHitBySegmentBounds(const FVector& CameraLoc, const FVector& PawnLoc, TSet<AActor*>& InOutWallsHit)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ATheHouseSmartWall> It(World); It; ++It)
	{
		ATheHouseSmartWall* Wall = *It;
		if (!IsValid(Wall) || !Wall->ActorHasTag(FName("SeeThroughWall")))
		{
			continue;
		}

		FBox Box = Wall->GetComponentsBoundingBox(true);
		if (!Box.IsValid || Box.GetVolume() <= 0.f)
		{
			continue;
		}

		Box = Box.ExpandBy(OcclusionBoundsPadding);
		const FVector StartToEnd = PawnLoc - CameraLoc;
		if (FMath::LineBoxIntersection(Box, CameraLoc, PawnLoc, StartToEnd))
		{
			InOutWallsHit.Add(Wall);
			if (!OccludedActors.Contains(Wall))
			{
				OccludedActors.Add(Wall, 0.0f);
			}
		}
	}
}

void ATheHouseCameraPawn::ApplyFadeToActor(AActor* Actor, float Value)
{
	if (!Actor) return;

	// Optimization: If it's a Smart Wall, let it handle itself (including collision)
	if (ATheHouseSmartWall* SmartWall = Cast<ATheHouseSmartWall>(Actor))
	{
		SmartWall->SetWallOpacity(Value);
		return;
	}

	// Fallback for generic actors
	TArray<UStaticMeshComponent*> MeshComponents;
	Actor->GetComponents<UStaticMeshComponent>(MeshComponents);

	for (UStaticMeshComponent* Mesh : MeshComponents)
	{
		if (!Mesh) continue;

		// Iterate through all material slots
		int32 NumMaterials = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; i++)
		{
			UMaterialInterface* Mat = Mesh->GetMaterial(i);
			if (!Mat) continue;

			// Ensure we have a Dynamic Material Instance
			UMaterialInstanceDynamic* DMI = Cast<UMaterialInstanceDynamic>(Mat);
			if (!DMI)
			{
				// Only Create if we haven't already (Check prevents constant recreating)
				DMI = Mesh->CreateDynamicMaterialInstance(i, Mat);
			}

			if (DMI)
			{
				DMI->SetScalarParameterValue(MaterialFadeParameterName, Value);
			}
		}
	}
}
