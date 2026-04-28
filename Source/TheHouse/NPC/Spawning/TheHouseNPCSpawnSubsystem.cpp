#include "TheHouse/NPC/Spawning/TheHouseNPCSpawnSubsystem.h"

#include "TheHouse/NPC/TheHouseNPCCharacter.h"

#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

static constexpr ECollisionChannel TheHouse_SpawnOcclusionTraceChannel = ECC_Visibility;

void UTheHouseNPCSpawnSubsystem::GatherSpawnPointsByUsage(
	const ETheHouseNPCSpawnPointUsage Usage,
	TArray<ATheHouseNPCSpawnPoint*>& Out) const
{
	Out.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<ATheHouseNPCSpawnPoint> It(World); It; ++It)
	{
		ATheHouseNPCSpawnPoint* P = *It;
		if (IsValid(P) && P->Usage == Usage && P->Weight > 0.f)
		{
			Out.Add(P);
		}
	}
}

void UTheHouseNPCSpawnSubsystem::GetSpawnPointsByUsage(
	const ETheHouseNPCSpawnPointUsage Usage,
	TArray<ATheHouseNPCSpawnPoint*>& OutPoints) const
{
	GatherSpawnPointsByUsage(Usage, OutPoints);
}

ATheHouseNPCSpawnPoint* UTheHouseNPCSpawnSubsystem::PickWeightedRandom(const TArray<ATheHouseNPCSpawnPoint*>& Points) const
{
	double TotalW = 0.0;
	for (ATheHouseNPCSpawnPoint* P : Points)
	{
		if (IsValid(P) && P->Weight > 0.f)
		{
			TotalW += (double)P->Weight;
		}
	}
	if (TotalW <= 0.0)
	{
		return nullptr;
	}

	const double Roll = FMath::FRandRange(0.0, TotalW);
	double Acc = 0.0;
	for (ATheHouseNPCSpawnPoint* P : Points)
	{
		if (!IsValid(P) || P->Weight <= 0.f)
		{
			continue;
		}
		Acc += (double)P->Weight;
		if (Roll <= Acc)
		{
			return P;
		}
	}
	return Points.Num() > 0 ? Points.Last() : nullptr;
}

FVector UTheHouseNPCSpawnSubsystem::ResolveSpawnLocationFromPoint(ATheHouseNPCSpawnPoint* Point) const
{
	if (!IsValid(Point))
	{
		return FVector::ZeroVector;
	}

	UWorld* World = GetWorld();
	FVector Loc = Point->GetSpawnWorldLocation();

	if (Point->bProjectToNav && World)
	{
		if (UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			FNavLocation NavLoc;
			const FVector Extent(400.f, 400.f, 900.f);
			if (NavSys->ProjectPointToNavigation(Loc, NavLoc, Extent))
			{
				Loc = NavLoc.Location;
			}
		}
	}

	return Loc;
}

bool UTheHouseNPCSpawnSubsystem::IsWorldLocationTooCloseToView(
	APlayerController* ViewController,
	const FVector& WorldLocation,
	const float ScreenMarginPx,
	FVector* OutCamLoc) const
{
	if (!IsValid(ViewController))
	{
		return false;
	}

	FVector CamLoc;
	FRotator CamRot;
	ViewController->GetPlayerViewPoint(CamLoc, CamRot);
	if (OutCamLoc)
	{
		*OutCamLoc = CamLoc;
	}

	// Si le point est derrière la caméra, on le considère safe visuellement.
	const FVector Dir = (WorldLocation - CamLoc);
	const float DirLenSq = Dir.SizeSquared();
	if (DirLenSq <= KINDA_SMALL_NUMBER)
	{
		return true;
	}
	const FVector Forward = CamRot.Vector();
	const float Dot = FVector::DotProduct(Forward, Dir.GetSafeNormal());
	if (Dot <= 0.f)
	{
		return false;
	}

	int32 VX = 0, VY = 0;
	ViewController->GetViewportSize(VX, VY);
	if (VX <= 0 || VY <= 0)
	{
		return false;
	}

	FVector2D Screen;
	const bool bProjected = ViewController->ProjectWorldLocationToScreen(WorldLocation, Screen, /*bPlayerViewportRelative*/ true);
	if (!bProjected)
	{
		// Projection impossible = en pratique, hors champ.
		return false;
	}

	const float MinX = -ScreenMarginPx;
	const float MinY = -ScreenMarginPx;
	const float MaxX = (float)VX + ScreenMarginPx;
	const float MaxY = (float)VY + ScreenMarginPx;

	const bool bInsideExpanded =
		Screen.X >= MinX && Screen.X <= MaxX &&
		Screen.Y >= MinY && Screen.Y <= MaxY;

	return bInsideExpanded;
}

bool UTheHouseNPCSpawnSubsystem::IsOccludedFromCamera(APlayerController* ViewController, const FVector& WorldLocation) const
{
	if (!IsValid(ViewController))
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return true;
	}

	FVector CamLoc;
	FRotator CamRot;
	ViewController->GetPlayerViewPoint(CamLoc, CamRot);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TheHouseNPCSpawnOcclusion), /*bTraceComplex*/ true);
	if (AActor* ViewTarget = ViewController->GetViewTarget())
	{
		Params.AddIgnoredActor(ViewTarget);
	}

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, CamLoc, WorldLocation, TheHouse_SpawnOcclusionTraceChannel, Params);
	if (!bHit)
	{
		// Rien entre caméra et point => visible => pas acceptable si on veut de l'occlusion.
		return false;
	}

	// Si le premier hit est “près” du point (on tape le sol exactement au point), on considère visible.
	const float DistToPointSq = FVector::DistSquared(Hit.ImpactPoint, WorldLocation);
	return DistToPointSq > FMath::Square(60.f);
}

ATheHouseNPCCharacter* UTheHouseNPCSpawnSubsystem::SpawnOneNPCAtLocation(
	TSubclassOf<ATheHouseNPCCharacter> NPCClass,
	const FVector& WorldLocation,
	const FRotator& WorldRotation)
{
	UWorld* World = GetWorld();
	if (!World || !NPCClass)
	{
		return nullptr;
	}

	FTransform Xf(WorldRotation, WorldLocation);
	AActor* Deferred = UGameplayStatics::BeginDeferredActorSpawnFromClass(
		World,
		NPCClass,
		Xf,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn,
		/*Owner*/ nullptr);

	ATheHouseNPCCharacter* Npc = Cast<ATheHouseNPCCharacter>(Deferred);
	if (!Npc)
	{
		return nullptr;
	}

	UGameplayStatics::FinishSpawningActor(Npc, Xf);
	return Npc;
}

TArray<ATheHouseNPCCharacter*> UTheHouseNPCSpawnSubsystem::SpawnNPCs_CasinoRandom(
	TSubclassOf<ATheHouseNPCCharacter> NPCClass,
	int32 Count)
{
	TArray<ATheHouseNPCCharacter*> Out;
	Count = FMath::Max(0, Count);
	if (!NPCClass || Count == 0)
	{
		return Out;
	}

	TArray<ATheHouseNPCSpawnPoint*> Points;
	GatherSpawnPointsByUsage(ETheHouseNPCSpawnPointUsage::CasinoEntrance, Points);
	if (Points.Num() == 0)
	{
		return Out;
	}

	Out.Reserve(Count);
	for (int32 i = 0; i < Count; ++i)
	{
		ATheHouseNPCSpawnPoint* Pick = PickWeightedRandom(Points);
		if (!IsValid(Pick))
		{
			continue;
		}
		const FVector Loc = ResolveSpawnLocationFromPoint(Pick);
		const FRotator Rot = Pick->GetActorRotation();
		if (ATheHouseNPCCharacter* Npc = SpawnOneNPCAtLocation(NPCClass, Loc, Rot))
		{
			Out.Add(Npc);
		}
	}

	return Out;
}

TArray<ATheHouseNPCCharacter*> UTheHouseNPCSpawnSubsystem::SpawnNPCs_CityImmersive(
	TSubclassOf<ATheHouseNPCCharacter> NPCClass,
	int32 Count,
	APlayerController* ViewController,
	const FTheHouseCityImmersiveSpawnRules& Rules)
{
	TArray<ATheHouseNPCCharacter*> Out;
	Count = FMath::Max(0, Count);
	if (!NPCClass || Count == 0 || !IsValid(ViewController))
	{
		return Out;
	}

	TArray<ATheHouseNPCSpawnPoint*> Points;
	GatherSpawnPointsByUsage(ETheHouseNPCSpawnPointUsage::CityImmersive, Points);
	if (Points.Num() == 0)
	{
		return Out;
	}

	Out.Reserve(Count);

	for (int32 SpawnIdx = 0; SpawnIdx < Count; ++SpawnIdx)
	{
		ATheHouseNPCSpawnPoint* Chosen = nullptr;
		FVector ChosenLoc = FVector::ZeroVector;

		for (int32 TryIdx = 0; TryIdx < FMath::Max(1, Rules.MaxCandidateTries); ++TryIdx)
		{
			ATheHouseNPCSpawnPoint* Candidate = PickWeightedRandom(Points);
			if (!IsValid(Candidate))
			{
				continue;
			}

			const FVector CandidateLoc = ResolveSpawnLocationFromPoint(Candidate);

			FVector CamLoc;
			(void)IsWorldLocationTooCloseToView(ViewController, CandidateLoc, Rules.ScreenMarginPx, &CamLoc);
			const float DistUU = FVector::Dist(CamLoc, CandidateLoc);
			if (Rules.MinDistanceFromCameraUU > 0.f && DistUU < Rules.MinDistanceFromCameraUU)
			{
				continue;
			}
			if (Rules.MaxDistanceFromCameraUU > 0.f && DistUU > Rules.MaxDistanceFromCameraUU)
			{
				continue;
			}

			// Règle principale : hors écran (avec marge pour éviter pop au mouvement caméra).
			if (IsWorldLocationTooCloseToView(ViewController, CandidateLoc, Rules.ScreenMarginPx))
			{
				continue;
			}

			// Optionnel : exige occlusion.
			if (Rules.bRequireOccludedFromCamera && !IsOccludedFromCamera(ViewController, CandidateLoc))
			{
				continue;
			}

			Chosen = Candidate;
			ChosenLoc = CandidateLoc;
			break;
		}

		// Fallback (si rien n’a passé les règles) : on prend un point random quand même,
		// mais on garde au moins la contrainte "hors écran" si possible.
		if (!Chosen)
		{
			for (int32 TryIdx = 0; TryIdx < 10; ++TryIdx)
			{
				ATheHouseNPCSpawnPoint* Candidate = PickWeightedRandom(Points);
				if (!IsValid(Candidate))
				{
					continue;
				}
				const FVector CandidateLoc = ResolveSpawnLocationFromPoint(Candidate);
				if (!IsWorldLocationTooCloseToView(ViewController, CandidateLoc, Rules.ScreenMarginPx))
				{
					Chosen = Candidate;
					ChosenLoc = CandidateLoc;
					break;
				}
			}
		}
		if (!Chosen)
		{
			Chosen = PickWeightedRandom(Points);
			ChosenLoc = ResolveSpawnLocationFromPoint(Chosen);
		}

		if (!IsValid(Chosen))
		{
			continue;
		}

		const FRotator Rot = Chosen->GetActorRotation();
		if (ATheHouseNPCCharacter* Npc = SpawnOneNPCAtLocation(NPCClass, ChosenLoc, Rot))
		{
			Out.Add(Npc);
		}
	}

	return Out;
}

