#include "TheHouse/NPC/Spawning/TheHouseBuildingPortal.h"

#include "TheHouse/NPC/Persistence/TheHouseNPCPersistenceSubsystem.h"
#include "TheHouse/NPC/Spawning/TheHouseNPCSpawnPoint.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouse/Player/TheHouseFPSCharacter.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogTheHouseBuildingPortal, Log, All);

namespace TheHouseBuildingPortalInternal
{
	static void GetTeleportTransformFromAnchor(const AActor* Anchor, FVector& OutLoc, FRotator& OutRot)
	{
		if (!IsValid(Anchor))
		{
			OutLoc = FVector::ZeroVector;
			OutRot = FRotator::ZeroRotator;
			return;
		}

		if (const ATheHouseNPCSpawnPoint* SP = Cast<ATheHouseNPCSpawnPoint>(Anchor))
		{
			OutLoc = SP->GetSpawnWorldLocation();
			OutRot = SP->GetActorRotation();
			return;
		}

		OutLoc = Anchor->GetActorLocation();
		OutRot = Anchor->GetActorRotation();
	}
} // namespace TheHouseBuildingPortalInternal

ATheHouseBuildingPortal::ATheHouseBuildingPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetBoxExtent(FVector(160.f, 160.f, 200.f));
	// Profil « Trigger » (Config/DefaultEngine.ini) : overlap Pawn, WorldDynamic, PhysicsBody, etc.
	// L’ancienne config manuelle (Pawn seul + reste en Ignore) ne recouvrait pas toutes les capsules / object types.
	Trigger->SetCollisionProfileName(TEXT("Trigger"));
	Trigger->SetGenerateOverlapEvents(true);
}

void ATheHouseBuildingPortal::BeginPlay()
{
	Super::BeginPlay();
	if (Trigger)
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &ATheHouseBuildingPortal::OnTriggerBeginOverlap);
	}
}

void ATheHouseBuildingPortal::OnTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	(void)OverlappedComponent;
	(void)OtherComp;
	(void)OtherBodyIndex;
	(void)bFromSweep;
	(void)SweepResult;

	if (BuildingId == NAME_None)
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	UE_LOG(LogTheHouseBuildingPortal, Verbose,
		TEXT("Overlap building=%s other=%s comp=%s"),
		*BuildingId.ToString(),
		OtherActor ? *OtherActor->GetName() : TEXT("(null)"),
		OtherComp ? *OtherComp->GetName() : TEXT("(null)"));
#endif

	if (ATheHouseNPCCharacter* NPC = Cast<ATheHouseNPCCharacter>(OtherActor))
	{
		TeleportOrSimulate(NPC, /*bEnter*/ bDirectionIsEntering);
		return;
	}

	if (bTeleportPlayerCharacter)
	{
		if (APawn* Pawn = Cast<APawn>(OtherActor))
		{
			TeleportPlayerCharacterIfNeeded(Pawn, bDirectionIsEntering);
		}
	}
}

void ATheHouseBuildingPortal::TeleportOrSimulate(ATheHouseNPCCharacter* NPC, const bool bEnter) const
{
	if (!NPC)
	{
		return;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTheHouseNPCPersistenceSubsystem* Persist = GI->GetSubsystem<UTheHouseNPCPersistenceSubsystem>())
		{
			Persist->MoveNPCToBuilding(NPC, BuildingId, /*bInside*/ bEnter, /*bTeleportIfPossible*/ bTeleportIfPossible);
		}
	}

	if (!bTeleportIfPossible)
	{
		// MoveNPCToBuilding() s’occupe du destroy si nécessaire.
		return;
	}

	AActor* const TargetPoint = bEnter ? InteriorPoint : ExteriorPoint;
	if (!IsValid(TargetPoint))
	{
		return;
	}

	FVector TargetLoc;
	FRotator TargetRot;
	TheHouseBuildingPortalInternal::GetTeleportTransformFromAnchor(TargetPoint, TargetLoc, TargetRot);
	if (!NPC->TeleportTo(TargetLoc, TargetRot, false, false))
	{
		// Repli si pénétration / sweep refuse (ex. capsule vs sol)
		NPC->SetActorLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTheHouseBuildingPortal, Warning, TEXT("TeleportTo failed for NPC %s -> retry SetActorLocationAndRotation"),
			*GetNameSafe(NPC));
#endif
	}
}

void ATheHouseBuildingPortal::TeleportPlayerCharacterIfNeeded(APawn* Pawn, const bool bEnter) const
{
	if (!IsValid(Pawn) || !Pawn->IsPlayerControlled())
	{
		return;
	}

	// Option : seulement le mode FPS (évite de téléporter le pawn RTS caméra par erreur).
	if (!Cast<ATheHouseFPSCharacter>(Pawn))
	{
		return;
	}

	if (!bTeleportIfPossible)
	{
		return;
	}

	AActor* TargetPoint = bEnter ? InteriorPoint : ExteriorPoint;
	if (!IsValid(TargetPoint))
	{
		return;
	}

	FVector TargetLoc;
	FRotator TargetRot;
	TheHouseBuildingPortalInternal::GetTeleportTransformFromAnchor(TargetPoint, TargetLoc, TargetRot);
	if (!Pawn->TeleportTo(TargetLoc, TargetRot, false, false))
	{
		Pawn->SetActorLocationAndRotation(TargetLoc, TargetRot, false, nullptr, ETeleportType::TeleportPhysics);
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTheHouseBuildingPortal, Warning, TEXT("TeleportTo failed for pawn %s -> retry SetActorLocationAndRotation"),
			*GetNameSafe(Pawn));
#endif
	}
}

