// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/Core/TheHouseHealthComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

UTheHouseHealthComponent::UTheHouseHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTheHouseHealthComponent::BeginPlay()
{
	Super::BeginPlay();

	Health = FMath::Clamp(Health, 0.f, MaxHealth);
	if (AActor* Owner = GetOwner())
	{
		Owner->OnTakeAnyDamage.AddDynamic(this, &UTheHouseHealthComponent::HandleAnyDamage);
	}
}

void UTheHouseHealthComponent::ResetHealthToMax()
{
	bIsDead = false;
	Health = MaxHealth;
	OnHealthChanged.Broadcast(Health);
}

void UTheHouseHealthComponent::Kill(AActor* DamageCauser, AController* InstigatorController)
{
	if (bIsDead)
	{
		return;
	}
	Health = 0.f;
	Die(DamageCauser, InstigatorController);
}

void UTheHouseHealthComponent::HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatedBy, AActor* DamageCauser)
{
	(void)DamagedActor;
	(void)DamageType;
	if (bIsDead || Damage <= 0.f)
	{
		return;
	}

	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	OnHealthChanged.Broadcast(Health);

	if (Health <= 0.f)
	{
		Die(DamageCauser, InstigatedBy);
	}
}

void UTheHouseHealthComponent::Die(AActor* DamageCauser, AController* InstigatorController)
{
	if (bIsDead)
	{
		return;
	}
	bIsDead = true;

	OnDeath.Broadcast(GetOwner(), DamageCauser, InstigatorController);

	if (bAutoRagdollOnDeath)
	{
		TryEnableRagdoll();
	}

	if (DestroyAfterDeathSeconds > 0.f)
	{
		if (AActor* Owner = GetOwner())
		{
			Owner->SetLifeSpan(DestroyAfterDeathSeconds);
		}
	}
}

void UTheHouseHealthComponent::TryEnableRagdoll()
{
	ACharacter* Char = Cast<ACharacter>(GetOwner());
	if (!Char)
	{
		return;
	}

	// Stop movement & input-like control.
	if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
	{
		Move->StopMovementImmediately();
		Move->DisableMovement();
	}
	Char->DetachFromControllerPendingDestroy();

	UCapsuleComponent* Cap = Char->GetCapsuleComponent();
	if (Cap)
	{
		Cap->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	USkeletalMeshComponent* Mesh = Char->GetMesh();
	if (Mesh)
	{
		Mesh->SetCollisionProfileName(TEXT("Ragdoll"));
		Mesh->SetSimulatePhysics(true);
		Mesh->WakeAllRigidBodies();
		Mesh->bBlendPhysics = true;
	}
}

