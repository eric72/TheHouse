// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/NPC/TheHouseNPCEjectRegionVolume.h"

#include "Components/BoxComponent.h"

ATheHouseNPCEjectRegionVolume::ATheHouseNPCEjectRegionVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	EjectBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EjectBox"));
	SetRootComponent(EjectBox);
	EjectBox->SetBoxExtent(FVector(300.f, 300.f, 150.f));
	EjectBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EjectBox->SetGenerateOverlapEvents(false);
}

bool ATheHouseNPCEjectRegionVolume::ContainsWorldLocation(const FVector& WorldLocation) const
{
	if (!EjectBox)
	{
		return false;
	}

	const FTransform BoxXf = EjectBox->GetComponentTransform();
	const FVector Local = BoxXf.InverseTransformPosition(WorldLocation);
	const FVector Ext = EjectBox->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Ext.X + KINDA_SMALL_NUMBER && FMath::Abs(Local.Y) <= Ext.Y + KINDA_SMALL_NUMBER
		&& FMath::Abs(Local.Z) <= Ext.Z + KINDA_SMALL_NUMBER;
}

bool ATheHouseNPCEjectRegionVolume::TryComputeEjectionExitWorldLocation(const FVector& WorldFromLocation, FVector& OutWorld) const
{
	if (!EjectBox || !ContainsWorldLocation(WorldFromLocation))
	{
		return false;
	}

	const FTransform BoxXf = EjectBox->GetComponentTransform();
	const FVector Local = BoxXf.InverseTransformPosition(WorldFromLocation);
	const FVector Ext = EjectBox->GetScaledBoxExtent();

	const FVector Ratio(
		Ext.X > KINDA_SMALL_NUMBER ? FMath::Abs(Local.X) / Ext.X : 0.f,
		Ext.Y > KINDA_SMALL_NUMBER ? FMath::Abs(Local.Y) / Ext.Y : 0.f,
		Ext.Z > KINDA_SMALL_NUMBER ? FMath::Abs(Local.Z) / Ext.Z : 0.f);

	int32 Major = 0;
	float Best = Ratio.X;
	if (Ratio.Y >= Best)
	{
		Major = 1;
		Best = Ratio.Y;
	}
	if (Ratio.Z >= Best)
	{
		Major = 2;
	}

	FVector LocalExit = Local;
	const float Push = FMath::Max(0.f, OutwardPushUU);
	LocalExit[Major] = FMath::Sign(Local[Major]) * (Ext[Major] + Push);
	const int32 A = (Major + 1) % 3;
	const int32 B = (Major + 2) % 3;
	LocalExit[A] = FMath::Clamp(Local[A], -Ext[A], Ext[A]);
	LocalExit[B] = FMath::Clamp(Local[B], -Ext[B], Ext[B]);

	OutWorld = BoxXf.TransformPosition(LocalExit);
	return true;
}
