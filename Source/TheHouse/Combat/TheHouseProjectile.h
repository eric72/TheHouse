// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TheHouseProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class THEHOUSE_API ATheHouseProjectile : public AActor
{
	GENERATED_BODY()

public:
	ATheHouseProjectile();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Combat")
	TObjectPtr<USphereComponent> Collision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Combat")
	TObjectPtr<UProjectileMovementComponent> Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat", meta=(ClampMin="0.0"))
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat", meta=(ClampMin="0.0"))
	float LifeSeconds = 5.f;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);
};

