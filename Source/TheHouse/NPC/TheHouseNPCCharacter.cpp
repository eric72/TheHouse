// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/NPC/TheHouseNPCCharacter.h"

#include "TheHouse/UI/TheHouseRTSUITypes.h"
#include "TheHouse/NPC/TheHouseNPCAIController.h"
#include "TheHouse/Core/TheHouseHealthComponent.h"
#include "TheHouse/Combat/TheHouseWeapon.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCArchetype.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCStaffArchetype.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCCustomerArchetype.h"
#include "TheHouse/NPC/TheHouseNPCSubsystem.h"
#include "TheHouse/NPC/Persistence/TheHouseNPCPersistenceSubsystem.h"
#include "TheHouse/Object/TheHouseObject.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/GameInstance.h"

namespace TheHouseNPCSelectionInternal
{
	/** Aligné sur `ATheHouseObject::HighlightStencilValue` pour le même post-process d’outline. */
	static constexpr uint8 SelectionStencilValue = 1;
	/** Même plafond que les objets : pas d’overlay sur des bounds démesurés. */
	static constexpr float MaxOverlayHalfExtentUU = 20000.f;
	static constexpr float OverlayMaxDrawDistanceUU = 1.e12f;

	static void ClearCustomDepthOnAllPrimitives(ATheHouseNPCCharacter* Self)
	{
		if (!Self)
		{
			return;
		}
		TArray<UPrimitiveComponent*> Prims;
		Self->GetComponents<UPrimitiveComponent>(Prims, /*bIncludeFromChildActors*/ true);
		for (UPrimitiveComponent* P : Prims)
		{
			if (P)
			{
				P->SetRenderCustomDepth(false);
			}
		}
	}

}

ATheHouseNPCCharacter::ATheHouseNPCCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = ATheHouseNPCAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);

	GetCharacterMovement()->MaxWalkSpeed = 400.f;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	HealthComponent = CreateDefaultSubobject<UTheHouseHealthComponent>(TEXT("HealthComponent"));
}

void ATheHouseNPCCharacter::SetPersistentId(const FGuid& NewId)
{
	PersistentId = NewId;
}

void ATheHouseNPCCharacter::ApplyPersistedRuntimeFields(const FTheHouseNPCPersistentState& State)
{
	StaffIdentityDisplayName = State.DisplayName;
	Wallet = State.Wallet;
	SetStaffMonthlySalary(State.StaffMonthlySalary);
	SetStaffStarRating(State.StaffStarRating);
	StaffEmploymentStartInGameSeconds = State.StaffEmploymentStartInGameSeconds;
	StaffLastSalaryPaidDayIndex = State.StaffLastSalaryPaidDayIndex;
	StaffMeshVariantRollIndex = State.StaffMeshVariantRollIndex;

	// Reprise action en cours (si implémentée).
	if (State.CurrentActionId != NAME_None && State.CurrentActionRemainingSeconds > 0.f)
	{
		ApplyPersistedCurrentActionSnapshot(
			State.CurrentActionId,
			State.CurrentActionRemainingSeconds,
			State.bHasCurrentActionTargetWorldLocation,
			State.CurrentActionTargetWorldLocation);
	}
}

void ATheHouseNPCCharacter::GetPersistedCurrentActionSnapshot_Implementation(
	FName& OutActionId,
	float& OutActionRemainingSeconds,
	bool& bOutHasTargetWorldLocation,
	FVector& OutTargetWorldLocation) const
{
	OutActionId = NAME_None;
	OutActionRemainingSeconds = 0.f;
	bOutHasTargetWorldLocation = false;
	OutTargetWorldLocation = FVector::ZeroVector;
}

void ATheHouseNPCCharacter::ApplyPersistedCurrentActionSnapshot_Implementation(
	FName ActionId,
	float ActionRemainingSeconds,
	bool bHasTargetWorldLocation,
	FVector TargetWorldLocation)
{
	(void)ActionId;
	(void)ActionRemainingSeconds;
	(void)bHasTargetWorldLocation;
	(void)TargetWorldLocation;
}

void ATheHouseNPCCharacter::TheHouse_TryRestoreRuntimeFromPersistenceSubsystem()
{
	if (!PersistentId.IsValid())
	{
		return;
	}
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return;
	}
	UTheHouseNPCPersistenceSubsystem* Persist = GI->GetSubsystem<UTheHouseNPCPersistenceSubsystem>();
	if (!Persist)
	{
		return;
	}

	FTheHouseNPCPersistentState Copy;
	if (!Persist->TryGetStateCopy(PersistentId, Copy))
	{
		return;
	}

	ApplyPersistedRuntimeFields(Copy);
}

void ATheHouseNPCCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Ensure an id exists even for placed-in-world NPCs (or non-persistence spawns).
	if (!PersistentId.IsValid())
	{
		PersistentId = FGuid::NewGuid();
	}

	ApplyArchetypeDefaults();
	TheHouse_TryRestoreRuntimeFromPersistenceSubsystem();

	// Spawn weapon (optional).
	if (DefaultWeaponClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		EquippedWeapon = GetWorld()->SpawnActor<ATheHouseWeapon>(DefaultWeaponClass, Params);
		if (EquippedWeapon && GetMesh())
		{
			EquippedWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				WeaponAttachSocketName);
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UTheHouseNPCSubsystem* Registry = World->GetSubsystem<UTheHouseNPCSubsystem>())
		{
			Registry->RegisterNPC(this);
		}

		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UTheHouseNPCPersistenceSubsystem* Persist = GI->GetSubsystem<UTheHouseNPCPersistenceSubsystem>())
			{
				if (!bStaffPlacementPreviewActor)
				{
					Persist->RegisterSpawnedNPC(this);
				}
			}
		}
	}
}

bool ATheHouseNPCCharacter::TryFireWeaponAtActor(AActor* TargetActor)
{
	if (!EquippedWeapon || !IsValid(TargetActor))
	{
		return false;
	}
	const FVector Dir = (TargetActor->GetActorLocation() - EquippedWeapon->GetMuzzleTransform().GetLocation()).GetSafeNormal();
	EquippedWeapon->FireInDirection(Dir);
	return true;
}

void ATheHouseNPCCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTheHouseNPCSubsystem* Registry = World->GetSubsystem<UTheHouseNPCSubsystem>())
		{
			Registry->UnregisterNPC(this);
		}

		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UTheHouseNPCPersistenceSubsystem* Persist = GI->GetSubsystem<UTheHouseNPCPersistenceSubsystem>())
			{
				if (!bStaffPlacementPreviewActor)
				{
					Persist->NotifyNPCDespawned(this);
				}
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

ETheHouseNPCCategory ATheHouseNPCCharacter::GetNPCCategory_Implementation() const
{
	if (NPCArchetype)
	{
		return NPCArchetype->Category;
	}
	return ETheHouseNPCCategory::Unassigned;
}

UTheHouseNPCArchetype* ATheHouseNPCCharacter::GetNPCArchetype_Implementation() const
{
	return NPCArchetype;
}

void ATheHouseNPCCharacter::RestoreSelectionOverlayStack()
{
	for (const FNPCSelectionOverlayRestore& Entry : NPCSelectionOverlayRestoreStack)
	{
		if (USkeletalMeshComponent* SkMesh = Entry.Mesh.Get())
		{
			SkMesh->SetOverlayMaterial(Entry.PrevOverlay.Get());
			if (Entry.bTouchedOverlayMaxDrawDistance)
			{
				SkMesh->SetOverlayMaterialMaxDrawDistance(Entry.PreviousOverlayMaxDrawDistance);
			}
		}
	}
	NPCSelectionOverlayRestoreStack.Empty();
}

void ATheHouseNPCCharacter::ApplySkeletalSelectionHighlights()
{
	UMaterialInstanceDynamic* OverlayMat = nullptr;
	if (RTSSelectionOverlayMaterial)
	{
		if (UMaterialInstanceDynamic* ExistingDyn = Cast<UMaterialInstanceDynamic>(RTSSelectionOverlayMaterial))
		{
			OverlayMat = ExistingDyn;
		}
		else
		{
			OverlayMat = UMaterialInstanceDynamic::Create(RTSSelectionOverlayMaterial, this);
		}
	}
	else
	{
		OverlayMat = TheHouse_GetSharedRTSSelectionCreamOverlayMID();
	}

	TArray<USkeletalMeshComponent*> SkMeshes;
	GetComponents<USkeletalMeshComponent>(SkMeshes, /*bIncludeFromChildActors*/ true);
	for (USkeletalMeshComponent* Sk : SkMeshes)
	{
		if (!Sk)
		{
			continue;
		}
		Sk->SetRenderCustomDepth(true);
		Sk->SetCustomDepthStencilValue(TheHouseNPCSelectionInternal::SelectionStencilValue);

		if (OverlayMat && Sk->Bounds.BoxExtent.GetMax() <= TheHouseNPCSelectionInternal::MaxOverlayHalfExtentUU)
		{
			FNPCSelectionOverlayRestore Restore;
			Restore.Mesh = Sk;
			Restore.PrevOverlay = Sk->GetOverlayMaterial();
			Restore.bTouchedOverlayMaxDrawDistance = true;
			Restore.PreviousOverlayMaxDrawDistance = Sk->GetOverlayMaterialMaxDrawDistance();
			NPCSelectionOverlayRestoreStack.Add(Restore);
			Sk->SetOverlayMaterial(OverlayMat);
			Sk->SetOverlayMaterialMaxDrawDistance(TheHouseNPCSelectionInternal::OverlayMaxDrawDistanceUU);
		}
	}
}

void ATheHouseNPCCharacter::OnSelect()
{
	RestoreSelectionOverlayStack();
	// Retirer le contour sur tout primitive (Box de debug/sélection, capsules, etc.) puis squelettes (+ overlay comme les objets).
	TheHouseNPCSelectionInternal::ClearCustomDepthOnAllPrimitives(this);
	ApplySkeletalSelectionHighlights();
}

void ATheHouseNPCCharacter::OnDeselect()
{
	RestoreSelectionOverlayStack();
	TheHouseNPCSelectionInternal::ClearCustomDepthOnAllPrimitives(this);
}

bool ATheHouseNPCCharacter::IsSelectable() const
{
	return bCanBeSelected;
}

bool ATheHouseNPCCharacter::IsStaffGuardNPC() const
{
	if (GetNPCCategory_Implementation() != ETheHouseNPCCategory::Staff)
	{
		return false;
	}

	const UTheHouseNPCStaffArchetype* Staff = Cast<UTheHouseNPCStaffArchetype>(NPCArchetype);
	static const FName GuardRoleId(TEXT("GUARD"));
	return Staff != nullptr && Staff->StaffRoleId == GuardRoleId;
}

void ATheHouseNPCCharacter::OnGuardAttackOrderIssued_Implementation(ATheHouseNPCCharacter* AttackTarget)
{
	(void)AttackTarget;
}

void ATheHouseNPCCharacter::ApplyArchetypeDefaults_Implementation()
{
	Wallet = 0.f;
	StaffMonthlySalary = 0.f;
	StaffStarRating = 0;

	if (!NPCArchetype)
	{
		return;
	}

	if (NPCArchetype->GetClass() == UTheHouseNPCArchetype::StaticClass())
	{
		UE_LOG(LogTemp, Warning, TEXT("TheHouse: %s utilise un Data Asset de classe racine UTheHouseNPCArchetype ; préférez Staff / Customer / Special."),
			*GetName());
	}

	if (const UTheHouseNPCStaffArchetype* Staff = Cast<UTheHouseNPCStaffArchetype>(NPCArchetype))
	{
		StaffMonthlySalary = FMath::Max(0.f, Staff->DefaultMonthlySalary);
		StaffStarRating = FMath::Clamp(Staff->DefaultStarRating, 1, 5);

		if (Staff->SkeletalMeshVariants.Num() > 0 && GetMesh())
		{
			int32 Idx = INDEX_NONE;
			if (StaffMeshVariantRollIndex >= 0 && StaffMeshVariantRollIndex < Staff->SkeletalMeshVariants.Num())
			{
				Idx = StaffMeshVariantRollIndex;
			}
			else
			{
				Idx = FMath::RandRange(0, Staff->SkeletalMeshVariants.Num() - 1);
			}
			if (USkeletalMesh* VariantMesh = Staff->SkeletalMeshVariants[Idx].Get())
			{
				GetMesh()->SetSkeletalMesh(VariantMesh);
			}
		}

		if (Staff->AnimationBlueprintOverride && GetMesh())
		{
			GetMesh()->SetAnimInstanceClass(Staff->AnimationBlueprintOverride);
		}

		StaffMeshVariantRollIndex = INDEX_NONE;
	}
	else if (const UTheHouseNPCCustomerArchetype* Customer = Cast<UTheHouseNPCCustomerArchetype>(NPCArchetype))
	{
		Wallet = FMath::Max(0.f, Customer->StartingWallet);
	}
}

void ATheHouseNPCCharacter::ApplyInitialHireFromRosterOffer(const FTheHouseNPCStaffRosterOffer& Offer)
{
	SetStaffMonthlySalary(Offer.MonthlySalary);
	if (Offer.StarRating > 0)
	{
		SetStaffStarRating(Offer.StarRating);
	}
	if (!Offer.DisplayName.IsEmpty())
	{
		StaffIdentityDisplayName = Offer.DisplayName;
	}
}

void ATheHouseNPCCharacter::SetStaffMonthlySalary(const float NewMonthlySalary)
{
	StaffMonthlySalary = FMath::Max(0.f, NewMonthlySalary);
}

void ATheHouseNPCCharacter::SetStaffStarRating(const int32 NewStars)
{
	StaffStarRating = FMath::Clamp(NewStars, 0, 5);
}

void ATheHouseNPCCharacter::MarkStaffEmployedAtInGameTime(const float InGameSeconds, const int32 DayIndex)
{
	StaffEmploymentStartInGameSeconds = FMath::Max(0.f, InGameSeconds);
	StaffLastSalaryPaidDayIndex = DayIndex;
}

float ATheHouseNPCCharacter::GetStaffStarProgressionMultiplier() const
{
	if (StaffStarRating <= 0)
	{
		return 1.f;
	}
	const int32 S = FMath::Clamp(StaffStarRating, 1, 5);
	return 1.f + 0.25f * float(S - 1);
}

bool ATheHouseNPCCharacter::TrySpendFromWallet(const float Amount)
{
	if (Amount <= 0.f)
	{
		return true;
	}
	if (Wallet < Amount)
	{
		return false;
	}
	Wallet -= Amount;
	return true;
}

void ATheHouseNPCCharacter::AddToWallet(const float Amount)
{
	if (Amount <= 0.f)
	{
		return;
	}
	Wallet += Amount;
}

#if WITH_EDITOR
void ATheHouseNPCCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropName == GET_MEMBER_NAME_CHECKED(ATheHouseNPCCharacter, NPCArchetype))
	{
		ApplyArchetypeDefaults();
	}
}
#endif
