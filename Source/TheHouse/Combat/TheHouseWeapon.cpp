// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/Combat/TheHouseWeapon.h"

#include "TheHouse/Combat/TheHouseProjectile.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

ATheHouseWeapon::ATheHouseWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FTransform ATheHouseWeapon::GetMuzzleTransform() const
{
	if (Mesh && MuzzleSocketName != NAME_None && Mesh->DoesSocketExist(MuzzleSocketName))
	{
		return Mesh->GetSocketTransform(MuzzleSocketName, RTS_World);
	}
	return Mesh ? Mesh->GetComponentTransform() : GetActorTransform();
}

void ATheHouseWeapon::FireInDirection(const FVector& WorldDirection)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Dir = WorldDirection.GetSafeNormal();
	const FTransform MuzzleXf = GetMuzzleTransform();
	const FVector SpawnLoc = MuzzleXf.GetLocation();
	const FRotator SpawnRot = Dir.Rotation();

	if (ProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = GetInstigator();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATheHouseProjectile* P = World->SpawnActor<ATheHouseProjectile>(ProjectileClass, SpawnLoc, SpawnRot, Params);
		if (P)
		{
			P->Damage = ProjectileDamage;
		}
		return;
	}

	if (!bTraceFallbackIfNoProjectileClass)
	{
		return;
	}

	// Fallback hitscan.
	const FVector End = SpawnLoc + Dir * 100000.f;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(TheHouseWeaponTrace), true, GetOwner());
	Q.AddIgnoredActor(this);
	Q.AddIgnoredActor(GetOwner());

	if (World->LineTraceSingleByChannel(Hit, SpawnLoc, End, ECC_Visibility, Q) && Hit.GetActor())
	{
		AController* InstigatorCtrl = nullptr;
		if (APawn* InstPawn = GetInstigator())
		{
			InstigatorCtrl = InstPawn->GetController();
		}

		UGameplayStatics::ApplyPointDamage(
			Hit.GetActor(),
			ProjectileDamage,
			Dir,
			Hit,
			InstigatorCtrl,
			GetOwner() ? GetOwner() : this,
			UDamageType::StaticClass());
	}
}

