// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TheHouseNPCAIController.generated.h"

class ATheHouseNPCCharacter;

/**
 * Point d’extension pour l’IA des PNJ (Perception, BrainComponent, Behaviour Tree, EQS…).
 * Le joueur humain utilise PlayerController ; les PNJ utilisent toujours une dérivation de
 * AIController pour rester compatibles avec NavMesh, AIModule et Blueprint.
 */
UCLASS()
class THEHOUSE_API ATheHouseNPCAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATheHouseNPCAIController();

	virtual void EndPlay(const EEndPlayReason::Type EndReason) override;

	/** Ordre RTS garde : approcher la cible et tirer (ligne + dégâts simples) jusqu’à ClearScriptedAttackOrder ou nouveau MoveTo. */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|NPC|Combat")
	void IssueAttackOrderOnScriptedTarget(ATheHouseNPCCharacter* Target);

	/** Arrête le tir scripté, le timer et le focus sur la cible d’attaque. */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|NPC|Combat")
	void ClearScriptedAttackOrder();

protected:
	UPROPERTY(Transient)
	TWeakObjectPtr<ATheHouseNPCCharacter> ScriptedAttackTarget;

	FTimerHandle ScriptedAttackFireTimer;

	/** Distance à laquelle l’IA considère être assez proche pour tirer (MoveTo). */
	UPROPERTY(EditAnywhere, Category = "TheHouse|NPC|Combat", meta = (ClampMin = "50.0"))
	float ScriptedAttackMoveAcceptanceRadiusUU = 280.f;

	/** Période entre deux rayons de tir scriptés. */
	UPROPERTY(EditAnywhere, Category = "TheHouse|NPC|Combat", meta = (ClampMin = "0.05"))
	float ScriptedAttackFireIntervalSeconds = 0.45f;

	/** Dégâts par tir (ApplyPointDamage). */
	UPROPERTY(EditAnywhere, Category = "TheHouse|NPC|Combat", meta = (ClampMin = "0.0"))
	float ScriptedAttackDamagePerShot = 8.f;

	/** Au-delà de cette distance (uu), les tirs scriptés sont suspendus (sans annuler l’ordre). */
	UPROPERTY(EditAnywhere, Category = "TheHouse|NPC|Combat", meta = (ClampMin = "0.0"))
	float ScriptedAttackMaxEngageDistanceUU = 1200.f;

	void TickScriptedAttackFire();
};
