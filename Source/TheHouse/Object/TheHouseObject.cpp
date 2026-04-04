#include "TheHouseObject.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

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
}

void ATheHouseObject::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetActorTickEnabled(bShowFootprintDebug);
	RefreshExclusionZonePreviewMesh();
	SetExclusionZonePreviewVisible(bShowExclusionZonePreview);
}

void ATheHouseObject::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(bShowFootprintDebug);
	RefreshExclusionZonePreviewMesh();
	SetExclusionZonePreviewVisible(bShowExclusionZonePreview);
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
