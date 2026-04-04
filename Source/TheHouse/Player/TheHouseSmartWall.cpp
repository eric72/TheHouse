#include "TheHouseSmartWall.h"
#include "UObject/ConstructorHelpers.h"

namespace TheHouseSmartWallPrivate
{
	static float GetWallHalfHeightWorld(const UStaticMeshComponent* Mesh)
	{
		if (!Mesh || !Mesh->GetStaticMesh())
		{
			return 50.f;
		}
		const float ExtentZ = Mesh->GetStaticMesh()->GetBounds().BoxExtent.Z;
		const float Sz = FMath::Abs(Mesh->GetRelativeScale3D().Z);
		return ExtentZ * Sz;
	}
}

bool ATheHouseSmartWall::ShouldEnableWallFillCap() const
{
	return bUseFillCap || (FillCapTexture != nullptr);
}

void ATheHouseSmartWall::EnsureWallFillCapMaterialInstances()
{
	if (!WallFillCapMesh || !WallMesh)
	{
		return;
	}

	const int32 NumMaterials = WallFillCapMesh->GetNumMaterials();
	if (NumMaterials <= 0)
	{
		return;
	}

	// Copie le matériau du mur au cap (slot 0).
	if (WallMesh->GetNumMaterials() > 0)
	{
		WallFillCapMesh->SetMaterial(0, WallMesh->GetMaterial(0));
	}

	// Créer/mettre à jour les DMIs.
	if (CachedFillCapDMIs.Num() != NumMaterials)
	{
		CachedFillCapDMIs.Empty();
		CachedFillCapDMIs.Reserve(NumMaterials);

		for (int32 i = 0; i < NumMaterials; i++)
		{
			UMaterialInterface* Mat = WallFillCapMesh->GetMaterial(i);
			if (Mat)
			{
				if (UMaterialInstanceDynamic* DMI = WallFillCapMesh->CreateDynamicMaterialInstance(i, Mat))
				{
					CachedFillCapDMIs.Add(DMI);
				}
			}
		}
	}
}

void ATheHouseSmartWall::PushWallFillCapMaterialParams()
{
	if (!WallMesh || CachedFillCapDMIs.Num() == 0)
	{
		return;
	}

	const float HalfH = TheHouseSmartWallPrivate::GetWallHalfHeightWorld(WallMesh);

	UTexture* UseTexture = FillCapTexture ? FillCapTexture : EngineBlackTexture;
	const FName TextureParam = bFillCapUseInteriorTexture ? FillCapInteriorTextureParameterName : FillCapMainTextureParameterName;

	for (UMaterialInstanceDynamic* DMI : CachedFillCapDMIs)
	{
		if (!DMI)
		{
			continue;
		}

		// Pas de dither/occlusion sur le cap : on garde l'opacité au maximum.
		DMI->SetScalarParameterValue(MaterialFadeParameterName, 0.f);
		DMI->SetScalarParameterValue(WallHalfHeightParamName, HalfH);

		if (UseTexture && TextureParam != NAME_None)
		{
			DMI->SetTextureParameterValue(TextureParam, UseTexture);
		}
	}
}

void ATheHouseSmartWall::UpdateWallFillCapTransform()
{
	if (!WallFillCapMesh || !WallMesh || !WallMesh->GetStaticMesh() || !WallFillCapMesh->GetStaticMesh())
	{
		return;
	}

	const FBoxSphereBounds WallBounds = WallMesh->GetStaticMesh()->GetBounds();
	const FVector WallAbsScale = WallMesh->GetRelativeScale3D().GetAbs();
	const FVector WallHalf = WallBounds.BoxExtent * WallAbsScale;

	const FVector WallRelLoc = WallMesh->GetRelativeLocation();
	const float WallTopZ = WallRelLoc.Z + WallHalf.Z;

	const float CapHalfZ = 0.5f * FMath::Max(FillCapThicknessCm, 0.0f);
	CachedFillCapHalfThicknessCm = CapHalfZ;
	if (CapHalfZ <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const FBoxSphereBounds CapBaseBounds = WallFillCapMesh->GetStaticMesh()->GetBounds();
	const FVector CapBaseHalf = CapBaseBounds.BoxExtent;
	if (CapBaseHalf.X <= KINDA_SMALL_NUMBER || CapBaseHalf.Y <= KINDA_SMALL_NUMBER || CapBaseHalf.Z <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float CapCenterZ = WallTopZ - CapHalfZ - FillCapZOverlapCm;

	WallFillCapMesh->SetRelativeLocation(FVector(WallRelLoc.X, WallRelLoc.Y, CapCenterZ));

	const float NewScaleX = WallHalf.X / CapBaseHalf.X;
	const float NewScaleY = WallHalf.Y / CapBaseHalf.Y;
	const float NewScaleZ = CapHalfZ / CapBaseHalf.Z;
	WallFillCapMesh->SetRelativeScale3D(FVector(NewScaleX, NewScaleY, NewScaleZ));
}

void ATheHouseSmartWall::UpdateWallFillCap()
{
	if (!WallFillCapMesh || !WallMesh)
	{
		return;
	}

	const bool bEnable = ShouldEnableWallFillCap() && WallMesh->GetStaticMesh() != nullptr;
	WallFillCapMesh->SetVisibility(bEnable, true);
	if (!bEnable)
	{
		return;
	}

	EnsureWallFillCapMaterialInstances();
	PushWallFillCapMaterialParams();
	UpdateWallFillCapTransform();
}

void ATheHouseSmartWall::PushWallMeshMaterialParams()
{
	if (!WallMesh || CachedDMIs.Num() == 0)
	{
		return;
	}
	const float HalfH = TheHouseSmartWallPrivate::GetWallHalfHeightWorld(WallMesh);
	for (UMaterialInstanceDynamic* DMI : CachedDMIs)
	{
		if (DMI)
		{
			DMI->SetScalarParameterValue(WallHalfHeightParamName, HalfH);
		}
	}
}

ATheHouseSmartWall::ATheHouseSmartWall()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create Root Scene Component (The Pivot)
	USceneComponent* RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// Create visible mesh (Attached to Root, can be moved)
	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(RootScene);
	WallMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore); // Let Camera Boom pass through

	// Cap (surface pleine) pour remplir la coupe quand le mur est réduit.
	WallFillCapMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallFillCapMesh"));
	WallFillCapMesh->SetupAttachment(RootScene);
	WallFillCapMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WallFillCapMesh->SetGenerateOverlapEvents(false);
	WallFillCapMesh->SetVisibility(false, true);

	// Mesh de base : cube (sera rescalé dynamiquement).
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh.Succeeded())
		{
			WallFillCapMesh->SetStaticMesh(CubeMesh.Object);
		}
	}

	// Texture noire de fallback.
	{
		static ConstructorHelpers::FObjectFinder<UTexture> BlackTex(TEXT("/Engine/EngineResources/Black.Black"));
		if (BlackTex.Succeeded())
		{
			EngineBlackTexture = BlackTex.Object;
		}
	}

	// Add the Critical Tag
	Tags.Add(FName("SeeThroughWall"));
}

void ATheHouseSmartWall::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyMeshBottomAlignment();
	UpdateWallFillCap();
}

void ATheHouseSmartWall::ApplyMeshBottomAlignment()
{
	if (!bAlignMeshBottomToActorOrigin || !WallMesh || !WallMesh->GetStaticMesh())
	{
		return;
	}

	const FBoxSphereBounds MB = WallMesh->GetStaticMesh()->GetBounds();
	const float HalfZ = MB.BoxExtent.Z;
	const FVector Sc = WallMesh->GetRelativeScale3D();

	FVector Loc = WallMesh->GetRelativeLocation();
	// Pivot au centre du mesh : bas à -HalfZ*Sz → on remonte pour aligner la base sur l'origine du root (sol)
	Loc.Z = HalfZ * Sc.Z + MeshVerticalOffset;
	WallMesh->SetRelativeLocation(Loc);
}

void ATheHouseSmartWall::EnsureInitialMeshTransformCached()
{
	if (bMeshTransformCached || !WallMesh)
	{
		return;
	}
	ApplyMeshBottomAlignment();
	InitialWallMeshScale = WallMesh->GetRelativeScale3D();
	InitialWallMeshLocation = WallMesh->GetRelativeLocation();
	bMeshTransformCached = true;
}

void ATheHouseSmartWall::BeginPlay()
{
	Super::BeginPlay();
	bMeshTransformCached = false;
	EnsureInitialMeshTransformCached();

	// Cache DMIs once at startup for performance
	if (WallMesh)
	{
		int32 NumMaterials = WallMesh->GetNumMaterials();
		for (int32 i = 0; i < NumMaterials; i++)
		{
			UMaterialInterface* Mat = WallMesh->GetMaterial(i);
			if (Mat)
			{
				UMaterialInstanceDynamic* DMI = WallMesh->CreateDynamicMaterialInstance(i, Mat);
				if (DMI)
				{
					CachedDMIs.Add(DMI);
				}
			}
		}
	}

	PushWallMeshMaterialParams();

	UpdateWallFillCap();
}

void ATheHouseSmartWall::SetWallOpacity(float Opacity)
{
	EnsureInitialMeshTransformCached();
	const float T = FMath::Clamp(Opacity, 0.f, 1.f);

	// 1. Sims-style: shrink mesh on local Z (occlusion 1 = short wall)
	if (bShrinkMeshHeightForOcclusion && WallMesh)
	{
		// Hauteur totale actuelle du mur en cm (monde)
		const float HalfH = TheHouseSmartWallPrivate::GetWallHalfHeightWorld(WallMesh);
		const float FullHeight = HalfH * 2.0f;

		// Si une hauteur minimale en cm est spécifiée, on la convertit en facteur d'échelle.
		float EffectiveMinScale = MinWallHeightScaleWhenOccluded;
		if (MinVisibleHeightCm > 0.f && FullHeight > KINDA_SMALL_NUMBER)
		{
			const float ClampedMinHeight = FMath::Clamp(MinVisibleHeightCm, 0.f, FullHeight);
			EffectiveMinScale = FMath::Clamp(ClampedMinHeight / FullHeight, 0.01f, 1.f);
		}

		const float NewZ = FMath::Lerp(InitialWallMeshScale.Z, InitialWallMeshScale.Z * EffectiveMinScale, T);
		WallMesh->SetRelativeScale3D(FVector(InitialWallMeshScale.X, InitialWallMeshScale.Y, NewZ));

		if (bCompensateCenterPivot && WallMesh->GetStaticMesh())
		{
			const float ExtentZ = WallMesh->GetStaticMesh()->GetBounds().BoxExtent.Z;
			const float ZLift = ExtentZ * (InitialWallMeshScale.Z - NewZ);
			WallMesh->SetRelativeLocation(FVector(InitialWallMeshLocation.X, InitialWallMeshLocation.Y, InitialWallMeshLocation.Z + ZLift));
		}
	}

	// Met à jour la surface de remplissage (si activée) après la réduction du mur.
	UpdateWallFillCap();

	// 2. Demi-hauteur monde pour le shader (pied du mur = 0 après Add dans le matériau)
	if (CachedDMIs.Num() > 0)
	{
		PushWallMeshMaterialParams();
	}

	// 3. Optional material scalar (same 0 = full wall, 1 = occluded)
	if (bDriveMaterialParameter)
	{
		for (UMaterialInstanceDynamic* DMI : CachedDMIs)
		{
			if (DMI)
			{
				// On envoie directement CameraFade = Opacity (0 = mur plein, 1 = occlusion max).
				DMI->SetScalarParameterValue(MaterialFadeParameterName, T);
			}
		}
	}

	// 4. ECC_Visibility — high occlusion = see-through for cursor
	if (WallMesh)
	{
		if (T > 0.8f)
		{
			WallMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		}
		else if (T < 0.2f)
		{
			WallMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		}
	}
}
