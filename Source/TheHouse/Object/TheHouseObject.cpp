#include "TheHouseObject.h"
#include "Algo/Sort.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MeshComponent.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "WorldCollision.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"

namespace TheHouseObjectInternal
{
	/** Cube BasicShapes : 100 uu de côté (extents 50). */
	static constexpr float EngineBasicCubeSideUU = 100.f;
	/** Au-delà : pas d’overlay (évite un sol / landscape entier en rouge). */
	static constexpr float MaxOverlayHalfExtentUU = 20000.f;
	/** Distance max d’overlay : évite le culling RTS quand la valeur par défaut est 0. */
	static constexpr float OverlayMaxDrawDistanceUU = 1.e12f;

	static int32 EffectivePlacementBlockerCap(int32 RawMax)
	{
		return (RawMax > 0) ? RawMax : 6;
	}
}

namespace TheHouseSelectionOverlayInternal
{
	/** MID partagé : teinte crème (blanc cassé) sur la base du matériau « placement valide », sans réutiliser l’asset tel quel sur la sélection. */
	static UMaterialInstanceDynamic* GetOrCreateCreamSelectionOverlayMID()
	{
		static TWeakObjectPtr<UMaterialInstanceDynamic> Cached;
		if (Cached.IsValid())
		{
			return Cached.Get();
		}

		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/Placement/M_Placement_Valid.M_Placement_Valid"));
		if (!Parent)
		{
			return nullptr;
		}

		UMaterialInstanceDynamic* Dyn = UMaterialInstanceDynamic::Create(Parent, GetTransientPackage());
		if (!Dyn)
		{
			return nullptr;
		}

		const FLinearColor Cream(0.94f, 0.93f, 0.91f, 0.38f);
		static const FName VectorParamNames[] = {
			FName(TEXT("Color")),
			FName(TEXT("TintColor")),
			FName(TEXT("BaseColor")),
			FName(TEXT("EmissiveColor")),
			FName(TEXT("Colour")),
			FName(TEXT("Tint")),
		};
		for (const FName& N : VectorParamNames)
		{
			Dyn->SetVectorParameterValue(N, Cream);
		}
		Dyn->SetScalarParameterValue(FName(TEXT("Opacity")), 0.38f);
		Dyn->SetScalarParameterValue(FName(TEXT("OpacityScale")), 0.38f);

		Cached = Dyn;
		return Dyn;
	}
}

UMaterialInstanceDynamic* TheHouse_GetSharedRTSSelectionCreamOverlayMID()
{
	return TheHouseSelectionOverlayInternal::GetOrCreateCreamSelectionOverlayMID();
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
	// Overlay bloqueurs : visible même sans post-process d’outline (même asset que « invalid » par défaut).
	if (!PlacementBlockerOverlayMaterial && DefaultInvalidMat.Succeeded())
	{
		PlacementBlockerOverlayMaterial = DefaultInvalidMat.Object;
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
	ClearPlacementBlockerActorCaches();

	const int32 BlockerCap = TheHouseObjectInternal::EffectivePlacementBlockerCap(MaxPlacementBlockerOutlines);

	// --- Autres objets casino (boîte d'exclusion AABB) ---
	{
		const FBox CandidateBox = GetLocalPlacementExclusionBox().TransformBy(WorldTransform);
		bool bAny = false;
		for (TActorIterator<ATheHouseObject> It(GetWorld()); It; ++It)
		{
			ATheHouseObject* Other = *It;
			if (!Other || Other == this)
			{
				continue;
			}

			const FBox OtherBox = Other->GetWorldPlacementExclusionBox();
			if (!CandidateBox.Intersect(OtherBox))
			{
				continue;
			}

			bAny = true;
			if (PlacementFeedbackObjectBlockers.Num() < BlockerCap)
			{
				PlacementFeedbackObjectBlockers.Add(Other);
			}
		}

		if (bAny)
		{
			PlacementState = EObjectPlacementState::OverlapsObject;
			return false;
		}
	}

	// --- Monde (une seule requête overlap : bool + liste bornée pour le feedback) ---
	if (TestFootprintOverlapsWorldAtWithBlockerList(WorldTransform, &PlacementFeedbackWorldBlockers, BlockerCap))
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
	// UE 5.7: IsVisualizationComponent() n'existe plus sur UPrimitiveComponent.
	// On saute le visualizer d'exclusion et tout composant editor-only.
	return Prim && (Prim == ExclusionZoneVisualizer || Prim->IsEditorOnly());
}

UMaterialInterface* ATheHouseObject::ResolveSelectionOverlayMaterial() const
{
	if (SelectionOverlayMaterial)
	{
		return SelectionOverlayMaterial;
	}
	if (UMaterialInstanceDynamic* Mid = TheHouseSelectionOverlayInternal::GetOrCreateCreamSelectionOverlayMID())
	{
		return Mid;
	}
	return nullptr;
}

UMaterialInterface* ATheHouseObject::ResolveBlockerOverlayMaterial() const
{
	if (PlacementBlockerOverlayMaterial)
	{
		return PlacementBlockerOverlayMaterial;
	}
	static TWeakObjectPtr<UMaterialInterface> Cached;
	if (!Cached.IsValid())
	{
		Cached = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/Placement/M_Placement_Invalid.M_Placement_Invalid"));
	}
	return Cached.Get();
}

void ATheHouseObject::ApplyHighlightToRenderers(bool bOn)
{
	if (!bOn)
	{
		for (const FSelectionOverlayRestore& Entry : SelectionOverlayRestoreStack)
		{
			if (UMeshComponent* Mesh = Entry.Mesh.Get())
			{
				Mesh->SetOverlayMaterial(Entry.PrevOverlay.Get());
				if (Entry.bTouchedOverlayMaxDrawDistance)
				{
					Mesh->SetOverlayMaterialMaxDrawDistance(Entry.PreviousOverlayMaxDrawDistance);
				}
			}
		}
		SelectionOverlayRestoreStack.Empty();
	}

	TArray<UPrimitiveComponent*> Primitives;
	GetComponents(Primitives, /*bIncludeFromChildActors*/ true);
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

	if (bOn)
	{
		SelectionOverlayRestoreStack.Empty();
		if (UMaterialInterface* OverlayMat = ResolveSelectionOverlayMaterial())
		{
			for (UPrimitiveComponent* Prim : Primitives)
			{
				if (!Prim || ShouldSkipHighlightOnPrimitive(Prim))
				{
					continue;
				}
				UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim);
				if (!MeshComp)
				{
					continue;
				}
				if (Prim->Bounds.BoxExtent.GetMax() > TheHouseObjectInternal::MaxOverlayHalfExtentUU)
				{
					continue;
				}

				FSelectionOverlayRestore Restore;
				Restore.Mesh = MeshComp;
				Restore.PrevOverlay = MeshComp->GetOverlayMaterial();
				Restore.bTouchedOverlayMaxDrawDistance = true;
				Restore.PreviousOverlayMaxDrawDistance = MeshComp->GetOverlayMaterialMaxDrawDistance();
				SelectionOverlayRestoreStack.Add(Restore);
				MeshComp->SetOverlayMaterial(OverlayMat);
				MeshComp->SetOverlayMaterialMaxDrawDistance(TheHouseObjectInternal::OverlayMaxDrawDistanceUU);
			}
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

	// En preview : vert/rouge uniquement sur la zone d’exclusion ; le mesh garde ses matériaux d’origine.
	if (bIsPreview)
	{
		RefreshPlacementBlockerHighlights();
	}
	else
	{
		ClearPlacementBlockerHighlights();
	}
	DrawPlacementBlockerWireBounds();
}

void ATheHouseObject::GetActorOutlineDebugBounds(AActor* Actor, FVector& OutCenter, FVector& OutExtent)
{
	OutCenter = FVector::ZeroVector;
	OutExtent = FVector::ZeroVector;
	if (!IsValid(Actor))
	{
		return;
	}
	if (ATheHouseObject* THO = Cast<ATheHouseObject>(Actor))
	{
		if (THO->ObjectMesh)
		{
			OutCenter = THO->ObjectMesh->Bounds.Origin;
			OutExtent = THO->ObjectMesh->Bounds.BoxExtent;
			return;
		}
	}
	// PNJ / FPS : éviter les bounds agrégées (capsule géante) utilisées pour le debug de sélection RTS.
	if (const ACharacter* Ch = Cast<ACharacter>(Actor))
	{
		if (const USkeletalMeshComponent* Sk = Ch->GetMesh())
		{
			const FBoxSphereBounds& MB = Sk->Bounds;
			OutCenter = MB.Origin;
			OutExtent = MB.BoxExtent;
			return;
		}
	}
	Actor->GetActorBounds(false, OutCenter, OutExtent, /*bIncludeFromChildActors*/ true);
}

void ATheHouseObject::DrawPlacementBlockerWireBounds()
{
	if (!GetWorld() || !bIsPreview)
	{
		return;
	}
	if (PlacementState != EObjectPlacementState::OverlapsObject && PlacementState != EObjectPlacementState::OverlapsWorld)
	{
		return;
	}

	const FColor RimColor(252, 248, 240);
	for (const TWeakObjectPtr<AActor>& W : PlacementBlockerHighlightTargets)
	{
		AActor* A = W.Get();
		if (!A || A == this)
		{
			continue;
		}
		FVector Origin, Extent;
		GetActorOutlineDebugBounds(A, Origin, Extent);
		if (Extent.GetMax() <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		DrawDebugBox(GetWorld(), Origin, Extent, FQuat::Identity, RimColor, false, 0.f, 0, 2.5f);
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
	return TestFootprintOverlapsWorldAtWithBlockerList(WorldTransform, nullptr, 0);
}

bool ATheHouseObject::TestFootprintOverlapsWorldAtWithBlockerList(
	const FTransform& WorldTransform,
	TArray<TObjectPtr<AActor>>* OutSortedUniqueBlockers,
	int32 MaxBlockersToStore) const
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

	if (!OutSortedUniqueBlockers || MaxBlockersToStore <= 0)
	{
		return World->OverlapBlockingTestByChannel(
			Center,
			Rotation,
			ECC_GameTraceChannel1, // PlacementBlocker
			Shape,
			Params
		);
	}

	// Chemin « feedback » : test bloquant d'abord (même coût qu'avant quand la pose est valide).
	if (!World->OverlapBlockingTestByChannel(
			Center,
			Rotation,
			ECC_GameTraceChannel1,
			Shape,
			Params))
	{
		OutSortedUniqueBlockers->Reset();
		return false;
	}

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(
		Overlaps,
		Center,
		Rotation,
		ECC_GameTraceChannel1, // PlacementBlocker
		Shape,
		Params
	);

	if (Overlaps.Num() == 0)
	{
		OutSortedUniqueBlockers->Reset();
		// Bloquant détecté mais aucun résultat multi (rare) : invalide sans liste.
		return true;
	}

	TMap<AActor*, float> ActorBestDistSq;
	for (const FOverlapResult& O : Overlaps)
	{
		AActor* A = O.GetActor();
		if (!A || A == this || A == PlacementWorldIgnoreActor)
		{
			continue;
		}

		const float D2 = FVector::DistSquared(A->GetActorLocation(), Center);
		if (float* Existing = ActorBestDistSq.Find(A))
		{
			if (D2 < *Existing)
			{
				*Existing = D2;
			}
		}
		else
		{
			ActorBestDistSq.Add(A, D2);
		}
	}

	TArray<TPair<AActor*, float>> Sorted;
	Sorted.Reserve(ActorBestDistSq.Num());
	for (const TPair<AActor*, float>& Pair : ActorBestDistSq)
	{
		Sorted.Emplace(Pair.Key, Pair.Value);
	}

	Algo::Sort(Sorted, [](const TPair<AActor*, float>& L, const TPair<AActor*, float>& R)
	{
		return L.Value < R.Value;
	});

	OutSortedUniqueBlockers->Reset();
	const int32 Limit = FMath::Min(MaxBlockersToStore, Sorted.Num());
	for (int32 i = 0; i < Limit; ++i)
	{
		OutSortedUniqueBlockers->Add(Sorted[i].Key);
	}

	return true;
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
		GetComponents(Primitives, /*bIncludeFromChildActors*/ true);
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
		ClearPlacementBlockerActorCaches();
		ClearPlacementBlockerHighlights();
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
	ClearPlacementBlockerActorCaches();
	ClearPlacementBlockerHighlights();

	// Safety: ensure the finalized actor is visible in game.
	SetActorHiddenInGame(false);
}

void ATheHouseObject::ApplyPreviewCollisionAndRendering(bool bEnablePreview)
{
	TArray<UPrimitiveComponent*> Primitives;
	GetComponents(Primitives, /*bIncludeFromChildActors*/ true);

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
	GetComponents(Meshes, /*bIncludeFromChildActors*/ true);
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
	GetComponents(Meshes, /*bIncludeFromChildActors*/ true);
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

void ATheHouseObject::ClearPlacementBlockerActorCaches()
{
	PlacementFeedbackObjectBlockers.Empty();
	PlacementFeedbackWorldBlockers.Empty();
}

void ATheHouseObject::TeardownPlacementBlockerVisualFeedback()
{
	ClearPlacementBlockerActorCaches();
	ClearPlacementBlockerHighlights();
}

void ATheHouseObject::ClearPlacementBlockerHighlights()
{
	for (FPlacementBlockedPrimRestoreState& Entry : PlacementBlockerPrimitiveRestoreStack)
	{
		if (UPrimitiveComponent* Prim = Entry.Primitive.Get())
		{
			if (Entry.bTouchedOverlay)
			{
				if (UMeshComponent* Mesh = Cast<UMeshComponent>(Prim))
				{
					Mesh->SetOverlayMaterial(Entry.PreviousOverlay.Get());
					if (Entry.bTouchedOverlayMaxDrawDistance)
					{
						Mesh->SetOverlayMaterialMaxDrawDistance(Entry.PreviousOverlayMaxDrawDistance);
					}
				}
			}
			Prim->SetRenderCustomDepth(Entry.bHadCustomDepth);
			Prim->SetCustomDepthStencilValue(Entry.StencilValue);
		}
	}
	PlacementBlockerPrimitiveRestoreStack.Empty();
	PlacementBlockerHighlightTargets.Empty();
}

bool ATheHouseObject::PlacementBlockerHighlightTargetsMatch(const TArray<AActor*>& Desired) const
{
	if (Desired.Num() != PlacementBlockerHighlightTargets.Num())
	{
		return false;
	}
	for (int32 i = 0; i < Desired.Num(); ++i)
	{
		if (Desired[i] != PlacementBlockerHighlightTargets[i].Get())
		{
			return false;
		}
	}
	return true;
}

void ATheHouseObject::ApplyBlockerOutlineToActor(AActor* Blocker)
{
	if (!Blocker || Blocker == this)
	{
		return;
	}

	UStaticMeshComponent* OtherExclusion = nullptr;
	if (ATheHouseObject* OtherObj = Cast<ATheHouseObject>(Blocker))
	{
		OtherExclusion = OtherObj->ExclusionZoneVisualizer;
	}

	TArray<UPrimitiveComponent*> Primitives;
	Blocker->GetComponents(Primitives, /*bIncludeFromChildActors*/ true);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || Prim == OtherExclusion || Prim->IsEditorOnly())
		{
			continue;
		}

		FPlacementBlockedPrimRestoreState Restore;
		Restore.Primitive = Prim;
		Restore.bHadCustomDepth = Prim->bRenderCustomDepth;
		Restore.StencilValue = static_cast<int32>(Prim->CustomDepthStencilValue);
		Restore.bTouchedOverlay = false;

		if (UMaterialInterface* OverlayMat = ResolveBlockerOverlayMaterial())
		{
			if (UMeshComponent* MeshComp = Cast<UMeshComponent>(Prim))
			{
				if (Prim->Bounds.BoxExtent.GetMax() <= TheHouseObjectInternal::MaxOverlayHalfExtentUU)
				{
					Restore.bTouchedOverlay = true;
					Restore.PreviousOverlay = MeshComp->GetOverlayMaterial();
					Restore.bTouchedOverlayMaxDrawDistance = true;
					Restore.PreviousOverlayMaxDrawDistance = MeshComp->GetOverlayMaterialMaxDrawDistance();
					MeshComp->SetOverlayMaterial(OverlayMat);
					MeshComp->SetOverlayMaterialMaxDrawDistance(TheHouseObjectInternal::OverlayMaxDrawDistanceUU);
				}
			}
		}

		PlacementBlockerPrimitiveRestoreStack.Add(Restore);

		Prim->SetRenderCustomDepth(true);
		Prim->SetCustomDepthStencilValue(HighlightStencilValue);
	}
}

void ATheHouseObject::RefreshPlacementBlockerHighlights()
{
	TArray<AActor*> Desired;
	if (bIsPreview &&
		(PlacementState == EObjectPlacementState::OverlapsObject || PlacementState == EObjectPlacementState::OverlapsWorld))
	{
		if (PlacementState == EObjectPlacementState::OverlapsObject)
		{
			for (ATheHouseObject* Obj : PlacementFeedbackObjectBlockers)
			{
				if (Obj && Obj != this)
				{
					Desired.Add(Obj);
				}
			}
		}
		else
		{
			for (AActor* A : PlacementFeedbackWorldBlockers)
			{
				if (A && A != this)
				{
					Desired.Add(A);
				}
			}
		}

		Algo::Sort(Desired, [](AActor* L, AActor* R)
		{
			return L < R;
		});
	}

	if (PlacementBlockerHighlightTargetsMatch(Desired))
	{
		return;
	}

	ClearPlacementBlockerHighlights();

	if (Desired.Num() == 0)
	{
		return;
	}

	for (AActor* A : Desired)
	{
		ApplyBlockerOutlineToActor(A);
	}

	PlacementBlockerHighlightTargets.Reserve(Desired.Num());
	for (AActor* A : Desired)
	{
		PlacementBlockerHighlightTargets.Add(A);
	}
}
