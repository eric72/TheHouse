// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TheHouseHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FTheHouseOnDeath, AActor*, Victim, AActor*, DamageCauser, AController*, InstigatorController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTheHouseOnHealthChanged, float, NewHealth);

/**
 * Composant PV simple (joueur + PNJ). S'appuie sur le système de dégâts UE (ApplyDamage / ApplyPointDamage).
 * Quand Health <= 0, déclenche OnDeath et peut activer le ragdoll automatiquement.
 */
UCLASS(ClassGroup=(TheHouse), meta=(BlueprintSpawnableComponent))
class THEHOUSE_API UTheHouseHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTheHouseHealthComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Health", meta=(ClampMin="1.0"))
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Health")
	float Health = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Health")
	bool bIsDead = false;

	/** Si true, passe le personnage en ragdoll à la mort (mesh sim physics, capsule désactivée). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Health|Death")
	bool bAutoRagdollOnDeath = true;

	/** Durée avant destruction auto. 0 = rester. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Health|Death", meta=(ClampMin="0.0"))
	float DestroyAfterDeathSeconds = 12.f;

	UPROPERTY(BlueprintAssignable, Category="TheHouse|Health")
	FTheHouseOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="TheHouse|Health|Death")
	FTheHouseOnDeath OnDeath;

	UFUNCTION(BlueprintCallable, Category="TheHouse|Health")
	void ResetHealthToMax();

	UFUNCTION(BlueprintCallable, Category="TheHouse|Health")
	void Kill(AActor* DamageCauser, AController* InstigatorController);

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void HandleAnyDamage(AActor* DamagedActor, float Damage, const class UDamageType* DamageType,
		AController* InstigatedBy, AActor* DamageCauser);

	void Die(AActor* DamageCauser, AController* InstigatorController);
	void TryEnableRagdoll();
};

