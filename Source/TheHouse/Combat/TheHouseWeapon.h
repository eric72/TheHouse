// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheHouseWeapon.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class ATheHouseProjectile;

UCLASS()
class THEHOUSE_API ATheHouseWeapon : public AActor
{
	GENERATED_BODY()

public:
	ATheHouseWeapon();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Combat")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Combat")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Nom du socket (sur Mesh) d'où sort le projectile. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat")
	FName MuzzleSocketName = FName(TEXT("Muzzle"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat")
	TSubclassOf<ATheHouseProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat", meta=(ClampMin="0.0"))
	float ProjectileDamage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat")
	bool bTraceFallbackIfNoProjectileClass = true;

	/** Tire vers une direction monde (caméra, focus IA…). */
	UFUNCTION(BlueprintCallable, Category="TheHouse|Combat")
	void FireInDirection(const FVector& WorldDirection);

	/** Donne la transform du muzzle (socket) si disponible. */
	UFUNCTION(BlueprintPure, Category="TheHouse|Combat")
	FTransform GetMuzzleTransform() const;
};

