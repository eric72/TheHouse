// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/Combat/TheHouseProjectile.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

ATheHouseProjectile::ATheHouseProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(3.5f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Block);
	Collision->SetNotifyRigidBodyCollision(true);
	RootComponent = Collision;

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->InitialSpeed = 8000.f;
	Movement->MaxSpeed = 8000.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 5.f;
}

void ATheHouseProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (LifeSeconds > 0.f)
	{
		SetLifeSpan(LifeSeconds);
	}

	if (Collision)
	{
		Collision->OnComponentHit.AddDynamic(this, &ATheHouseProjectile::OnHit);
	}
}

void ATheHouseProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	(void)HitComp;
	(void)OtherComp;
	(void)NormalImpulse;

	AActor* MyOwner = GetOwner();
	if (!OtherActor || OtherActor == this || OtherActor == MyOwner)
	{
		Destroy();
		return;
	}

	const FVector ShotDir = GetVelocity().GetSafeNormal();
	AController* InstigatorCtrl = nullptr;
	if (APawn* InstPawn = GetInstigator())
	{
		InstigatorCtrl = InstPawn->GetController();
	}

	UGameplayStatics::ApplyPointDamage(
		OtherActor,
		Damage,
		ShotDir,
		Hit,
		InstigatorCtrl,
		MyOwner ? MyOwner : this,
		UDamageType::StaticClass());

	Destroy();
}

