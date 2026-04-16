#include "TheHouseObject.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/EngineTypes.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Pawn.h"

namespace TheHouseObjectInternal
{
	/** Cube BasicShapes : 100 uu de côté (extents 50). */
	static constexpr float EngineBasicCubeSideUU = 100.f;
}

ATheHouseObject::ATheHouseObject()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	ObjectMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ObjectMesh"));
	ObjectMesh->SetupAttachment(RootComponent);
	ObjectMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ObjectMesh->SetGenerateOverlapEvents(true);
	ObjectMesh->SetCanEverAffectNavigation(true);
	// Sélection / menu contextuel RTS : GetHitResultUnderCursor utilise souvent ECC_Visibility.
	ObjectMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	ExclusionZoneVisualizer = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExclusionZoneVisualizer"));
	ExclusionZoneVisualizer->SetupAttachment(RootComponent);
	ExclusionZoneVisualizer->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ExclusionZoneVisualizer->SetCastShadow(false);
	ExclusionZoneVisualizer->bReceivesDecals = false;
	ExclusionZoneVisualizer->SetVisibility(false);
	ExclusionZoneVisualizer->SetHiddenInGame(true);
	ExclusionZoneVisualizer->SetCanEverAffectNavigation(false);
	ExclusionZoneVisualizer->SetGenerateOverlapEvents(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultCube(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (DefaultCube.Succeeded())
	{
		ExclusionZoneVisualizer->SetStaticMesh(DefaultCube.Object);
	}

	// Default placement materials (can be overridden in BP).
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultValidMat(
		TEXT("/Game/Materials/Placement/M_Placement_Valid.M_Placement_Valid"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultInvalidMat(
		TEXT("/Game/Materials/Placement/M_Placement_Invalid.M_Placement_Invalid"));

	if (!ValidPlacementMaterial && DefaultValidMat.Succeeded())
	{
		ValidPlacementMaterial = DefaultValidMat.Object;
	}
	if (!InvalidPlacementMaterial && DefaultInvalidMat.Succeeded())
	{
		InvalidPlacementMaterial = DefaultInvalidMat.Object;
	}
}

void ATheHouseObject::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetActorTickEnabled(bShowFootprintDebug);
	RefreshExclusionZonePreviewMesh();
	SetExclusionZonePreviewVisible(bShowExclusionZonePreview);

	// Keep occupants array aligned in editor too (runtime-only values will be null).
	NPCSlotOccupants.SetNum(NPCInteractionSlots.Num());
}

void ATheHouseObject::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(bShowFootprintDebug);
	RefreshExclusionZonePreviewMesh();

	// In game, the exclusion volume is only meant for placement preview.
	// Force it hidden unless we're explicitly in preview mode.
	if (!bIsPreview)
	{
		bShowExclusionZonePreview = false;
	}
	SetExclusionZonePreviewVisible(bIsPreview);

	// Ensure runtime occupants array is sized.
	NPCSlotOccupants.SetNum(NPCInteractionSlots.Num());
}

void ATheHouseObject::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	if (bShowFootprintDebug && GetWorld())
	{
		const FBox WorldBox = GetWorldPlacementExclusionBox();
		DrawDebugBox(GetWorld(), WorldBox.GetCenter(), WorldBox.GetExtent(), FootprintDebugColor, false, 0.f, 0, 2.f);
	}
#endif
}

FBox ATheHouseObject::GetLocalPlacementExclusionBox() const
{
	const FVector Min = PlacementExclusionCenterOffset - PlacementExclusionHalfExtent;
	const FVector Max = PlacementExclusionCenterOffset + PlacementExclusionHalfExtent;
	return FBox(Min, Max);
}

FBox ATheHouseObject::GetWorldPlacementExclusionBox() const
{
	return GetLocalPlacementExclusionBox().TransformBy(GetActorTransform());
}

bool ATheHouseObject::TestFootprintOverlapsOthersAt(const FTransform& WorldTransform, AActor* IgnoreActor) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FBox CandidateBox = GetLocalPlacementExclusionBox().TransformBy(WorldTransform);

	for (TActorIterator<ATheHouseObject> It(World); It; ++It)
	{
		ATheHouseObject* Other = *It;
		if (!Other || Other == this || Other == IgnoreActor)
		{
			continue;
		}

		const FBox OtherBox = Other->GetWorldPlacementExclusionBox();
		if (CandidateBox.Intersect(OtherBox))
		{
			return true;
		}
	}

	return false;
}

bool ATheHouseObject::EvaluatePlacementAt(const FTransform& WorldTransform)
{
	if (TestFootprintOverlapsOthersAt(WorldTransform))
	{
		PlacementState = EObjectPlacementState::OverlapsObject;
		return false;
	}

	if (TestFootprintOverlapsWorldAt(WorldTransform))
	{
		PlacementState = EObjectPlacementState::OverlapsWorld;
		return false;
	}

	PlacementState = EObjectPlacementState::Valid;
	return true;
}

void ATheHouseObject::RefreshExclusionZonePreviewMesh()
{
	if (!ExclusionZoneVisualizer)
	{
		return;
	}

	if (ExclusionZonePreviewMesh)
	{
		ExclusionZoneVisualizer->SetStaticMesh(ExclusionZonePreviewMesh);
	}

	ExclusionZoneVisualizer->SetRelativeLocation(PlacementExclusionCenterOffset);

	const FVector He = PlacementExclusionHalfExtent;
	const float S = TheHouseObjectInternal::EngineBasicCubeSideUU;
	const FVector Scale((He.X * 2.f) / S, (He.Y * 2.f) / S, (He.Z * 2.f) / S);
	ExclusionZoneVisualizer->SetRelativeScale3D(Scale);

	ApplyExclusionZoneMaterial();
}

void ATheHouseObject::ApplyExclusionZoneMaterial()
{
	if (ExclusionZoneVisualizer && ExclusionZonePreviewMaterial)
	{
		ExclusionZoneVisualizer->SetMaterial(0, ExclusionZonePreviewMaterial);
	}
}

void ATheHouseObject::SetExclusionZonePreviewVisible(bool bVisible)
{
	bShowExclusionZonePreview = bVisible;
	if (!ExclusionZoneVisualizer)
	{
		return;
	}
	ExclusionZoneVisualizer->SetVisibility(bVisible);
	ExclusionZoneVisualizer->SetHiddenInGame(!bVisible);
}

void ATheHouseObject::SetHighlighted(bool bInHighlighted)
{
	if (bHighlighted == bInHighlighted)
	{
		return;
	}
	bHighlighted = bInHighlighted;
	ApplyHighlightToRenderers(bHighlighted);
}

bool ATheHouseObject::ShouldSkipHighlightOnPrimitive(UPrimitiveComponent* Prim) const
{
	return Prim && (Prim == ExclusionZoneVisualizer || Prim->IsVisualizationComponent());
}

void ATheHouseObject::ApplyHighlightToRenderers(bool bOn)
{
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents(Primitives);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || ShouldSkipHighlightOnPrimitive(Prim))
		{
			continue;
		}
		Prim->SetRenderCustomDepth(bOn);
		if (bOn)
		{
			Prim->SetCustomDepthStencilValue(HighlightStencilValue);
		}
	}
}

void ATheHouseObject::OnSelect()
{
	SetHighlighted(true);
}

void ATheHouseObject::OnDeselect()
{
	SetHighlighted(false);
}

bool ATheHouseObject::IsSelectable() const
{
	return true;
}

APawn* ATheHouseObject::GetNPCSlotOccupant(int32 SlotIndex) const
{
	if (!NPCSlotOccupants.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}
	return NPCSlotOccupants[SlotIndex];
}

bool ATheHouseObject::GetNPCSlotWorldTransform(int32 SlotIndex, FTransform& OutSlotWorldTransform) const
{
	if (!NPCInteractionSlots.IsValidIndex(SlotIndex))
	{
		return false;
	}

	const FName Socket = NPCInteractionSlots[SlotIndex].SocketName;
	if (ObjectMesh && Socket != NAME_None && ObjectMesh->DoesSocketExist(Socket))
	{
		OutSlotWorldTransform = ObjectMesh->GetSocketTransform(Socket, RTS_World);
		return true;
	}

	OutSlotWorldTransform = GetActorTransform();
	return true;
}

bool ATheHouseObject::TryReserveNPCSlot(APawn* RequestingPawn, FName PurposeTag, int32& OutSlotIndex, FTransform& OutSlotWorldTransform)
{
	OutSlotIndex = INDEX_NONE;
	if (!RequestingPawn)
	{
		return false;
	}

	NPCSlotOccupants.SetNum(NPCInteractionSlots.Num());

	for (int32 i = 0; i < NPCInteractionSlots.Num(); ++i)
	{
		const FTheHouseNPCInteractionSlot& Slot = NPCInteractionSlots[i];
		if (PurposeTag != NAME_None && Slot.PurposeTag != PurposeTag)
		{
			continue;
		}
		if (NPCSlotOccupants.IsValidIndex(i) && NPCSlotOccupants[i] != nullptr)
		{
			continue;
		}

		FTransform T;
		if (!GetNPCSlotWorldTransform(i, T))
		{
			continue;
		}

		NPCSlotOccupants[i] = RequestingPawn;
		OutSlotIndex = i;
		OutSlotWorldTransform = T;
		return true;
	}

	return false;
}

bool ATheHouseObject::ReleaseNPCSlotByIndex(int32 SlotIndex, APawn* ReleasingPawn)
{
	if (!NPCSlotOccupants.IsValidIndex(SlotIndex))
	{
		return false;
	}

	APawn* Current = NPCSlotOccupants[SlotIndex];
	if (Current == nullptr)
	{
		return false;
	}

	if (ReleasingPawn && Current != ReleasingPawn)
	{
		return false;
	}

	NPCSlotOccupants[SlotIndex] = nullptr;
	return true;
}

int32 ATheHouseObject::ReleaseAllNPCSlotsForPawn(APawn* ReleasingPawn)
{
	if (!ReleasingPawn)
	{
		return 0;
	}

	int32 Released = 0;
	for (int32 i = 0; i < NPCSlotOccupants.Num(); ++i)
	{
		if (NPCSlotOccupants[i] == ReleasingPawn)
		{
			NPCSlotOccupants[i] = nullptr;
			++Released;
		}
	}
	return Released;
}

void ATheHouseObject::UpdatePlacementVisual()
{
	UMaterialInterface* Mat = nullptr;
	switch (PlacementState)
	{
		case EObjectPlacementState::Valid:
			Mat = ValidPlacementMaterial;
			break;
		default:
			Mat = InvalidPlacementMaterial;
			break;
	}

	if (ExclusionZoneVisualizer && Mat)
	{
		ExclusionZoneVisualizer->SetMaterial(0, Mat);
	}

	// Ne jamais afficher la zone d'exclusion en jeu hors mode preview (sauf debug explicitement activé).
	if (!bIsPreview && !bShowExclusionZonePreview)
	{
		SetExclusionZonePreviewVisible(false);
	}

	// En preview, on applique aussi le matériau à l'objet lui-même (ghost vert/rouge).
	if (bIsPreview)
	{
		ApplyPlacementMaterialToRenderers(Mat);
	}
}

void ATheHouseObject::SetPlacementState(EObjectPlacementState NewState)
{
	PlacementState = NewState;
	bPlacementValid = (NewState == EObjectPlacementState::Valid);
	UpdatePlacementVisual();
}

bool ATheHouseObject::TestFootprintOverlapsWorldAt(const FTransform& WorldTransform) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Center = WorldTransform.TransformPosition(PlacementExclusionCenterOffset);
	const FVector HalfExtent = PlacementExclusionHalfExtent;
	const FQuat Rotation = WorldTransform.GetRotation();

	FCollisionShape Shape = FCollisionShape::MakeBox(HalfExtent);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(PlacementWorldOverlap), /*bTraceComplex*/ false);
	Params.AddIgnoredActor(this);
	if (PlacementWorldIgnoreActor)
	{
		Params.AddIgnoredActor(PlacementWorldIgnoreActor);
	}

	return World->OverlapBlockingTestByChannel(
		Center,
		Rotation,
		ECC_GameTraceChannel1, // PlacementBlocker
		Shape,
		Params
	);
}

void ATheHouseObject::SetAsPreview(bool bInPreview)
{
	bIsPreview = bInPreview;

	ApplyPreviewCollisionAndRendering(bInPreview);

	// Preview should always remain visible (avoid BP/defaults hiding components).
	if (bInPreview)
	{
		SetActorHiddenInGame(false);
		TArray<UPrimitiveComponent*> Primitives;
		GetComponents(Primitives);
		for (UPrimitiveComponent* Prim : Primitives)
		{
			if (!Prim)
			{
				continue;
			}
			Prim->SetVisibility(true, true);
			Prim->SetHiddenInGame(false, true);
			// Force no shadow in preview (even if a BP re-enabled it).
			Prim->SetCastShadow(false);
			Prim->bCastHiddenShadow = false;
		}
	}

	// preview = tick placement actif possible
	SetActorTickEnabled(bInPreview);

	// Affiche le visualiseur de zone pendant le placement.
	SetExclusionZonePreviewVisible(bInPreview ? true : bShowExclusionZonePreview);

	if (bInPreview)
	{
		CacheOriginalMaterialsIfNeeded();
		UpdatePlacementVisual();
	}
	else
	{
		RestoreOriginalMaterials();
		SetExclusionZonePreviewVisible(bShowExclusionZonePreview);
	}
}

bool ATheHouseObject::SetPlacementTransform(const FTransform& Transform)
{
	SetActorTransform(Transform);

	const bool bValid = EvaluatePlacementAt(Transform);

	bPlacementValid = bValid;
	UpdatePlacementVisual();

	return bValid;
}

void ATheHouseObject::SetPlacementValid(bool bValid)
{
	bPlacementValid = bValid;
	UpdatePlacementVisual();
}

void ATheHouseObject::FinalizePlacement()
{
	bIsPreview = false;
	SetActorTickEnabled(false);

	ApplyPreviewCollisionAndRendering(false);
	RestoreOriginalMaterials();
	bShowExclusionZonePreview = false;
	SetExclusionZonePreviewVisible(false);

	// Safety: ensure the finalized actor is visible in game.
	SetActorHiddenInGame(false);
}

void ATheHouseObject::ApplyPreviewCollisionAndRendering(bool bEnablePreview)
{
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents(Primitives);

	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || ShouldSkipHighlightOnPrimitive(Prim))
		{
			continue;
		}

		if (bEnablePreview)
		{
			Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Prim->SetGenerateOverlapEvents(false);
			Prim->SetCastShadow(false);
			Prim->bCastHiddenShadow = false;
			Prim->bReceivesDecals = false;
		}
		else
		{
			// On restaure vers un état "normal" raisonnable sans présumer des profils exacts.
			// Les BPs peuvent reconfigurer finement ensuite.
			if (Prim->GetCollisionEnabled() == ECollisionEnabled::NoCollision)
			{
				Prim->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			}
			Prim->SetGenerateOverlapEvents(true);
			// Traces menu contextuel / curseur (ECC_Visibility) : le preview mettait NoCollision
			// sur tous les primitifs ; sans cela le mesh peut rester en Ignore sur Visibility.
			if (Prim->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
			{
				Prim->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
			}
		}
	}
}

void ATheHouseObject::CacheOriginalMaterialsIfNeeded()
{
	if (CachedOriginalMaterials.Num() > 0)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	GetComponents(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh || Mesh == ExclusionZoneVisualizer)
		{
			continue;
		}

		const int32 NumMats = Mesh->GetNumMaterials();
		TArray<UMaterialInterface*> Mats;
		Mats.Reserve(NumMats);
		for (int32 i = 0; i < NumMats; ++i)
		{
			Mats.Add(Mesh->GetMaterial(i));
		}
		CachedOriginalMaterials.Add(Mesh, MoveTemp(Mats));
	}
}

void ATheHouseObject::RestoreOriginalMaterials()
{
	for (auto& Pair : CachedOriginalMaterials)
	{
		UMeshComponent* Mesh = Pair.Key;
		if (!Mesh)
		{
			continue;
		}
		const TArray<UMaterialInterface*>& Mats = Pair.Value;
		for (int32 i = 0; i < Mats.Num(); ++i)
		{
			Mesh->SetMaterial(i, Mats[i]);
		}
	}
	CachedOriginalMaterials.Empty();
}

void ATheHouseObject::ApplyPlacementMaterialToRenderers(UMaterialInterface* MaterialOverride)
{
	if (!MaterialOverride)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	GetComponents(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh || Mesh == ExclusionZoneVisualizer)
		{
			continue;
		}

		const int32 NumMats = Mesh->GetNumMaterials();
		for (int32 i = 0; i < NumMats; ++i)
		{
			Mesh->SetMaterial(i, MaterialOverride);
		}
	}
}
