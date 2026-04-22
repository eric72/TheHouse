// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/NPC/TheHouseNPCAIController.h"

#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "Navigation/PathFollowingComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ATheHouseNPCAIController::ATheHouseNPCAIController()
{
	bWantsPlayerState = false;
}

void ATheHouseNPCAIController::EndPlay(const EEndPlayReason::Type EndReason)
{
	ClearScriptedAttackOrder();
	Super::EndPlay(EndReason);
}

void ATheHouseNPCAIController::ClearScriptedAttackOrder()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ScriptedAttackFireTimer);
	}
	ScriptedAttackTarget.Reset();
	ClearFocus(EAIFocusPriority::Gameplay);
}

void ATheHouseNPCAIController::IssueAttackOrderOnScriptedTarget(ATheHouseNPCCharacter* Target)
{
	if (!IsValid(Target) || Target == GetPawn())
	{
		return;
	}

	ClearScriptedAttackOrder();
	ScriptedAttackTarget = Target;

	ATheHouseNPCCharacter* SelfNpc = Cast<ATheHouseNPCCharacter>(GetPawn());
	if (SelfNpc)
	{
		SelfNpc->OnGuardAttackOrderIssued(Target);
	}

	SetFocus(Target, EAIFocusPriority::Gameplay);

	FAIMoveRequest Req;
	Req.SetGoalActor(Target);
	Req.SetAcceptanceRadius(ScriptedAttackMoveAcceptanceRadiusUU);
	Req.SetUsePathfinding(true);
	Req.SetAllowPartialPath(true);
	MoveTo(Req);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ScriptedAttackFireTimer,
			FTimerDelegate::CreateUObject(this, &ATheHouseNPCAIController::TickScriptedAttackFire),
			ScriptedAttackFireIntervalSeconds,
			true);
	}
}

void ATheHouseNPCAIController::TickScriptedAttackFire()
{
	ATheHouseNPCCharacter* SelfNpc = Cast<ATheHouseNPCCharacter>(GetPawn());
	ATheHouseNPCCharacter* TargetNpc = ScriptedAttackTarget.Get();
	if (!SelfNpc || !IsValid(TargetNpc))
	{
		ClearScriptedAttackOrder();
		return;
	}

	const float DistSq = FVector::DistSquared(SelfNpc->GetActorLocation(), TargetNpc->GetActorLocation());
	if (DistSq > FMath::Square(ScriptedAttackMaxEngageDistanceUU))
	{
		return;
	}

	const FVector Start = SelfNpc->GetPawnViewLocation();
	const FVector End = TargetNpc->GetActorLocation() + FVector(0.f, 0.f, 50.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(TheHouseGuardScriptedShot), false, SelfNpc);
	Params.AddIgnoredActor(SelfNpc);

	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return;
	}

	if (Hit.GetActor() != TargetNpc)
	{
		return;
	}

	const FVector ShotDir = (End - Start).GetSafeNormal();
	UGameplayStatics::ApplyPointDamage(
		TargetNpc,
		ScriptedAttackDamagePerShot,
		ShotDir,
		Hit,
		this,
		SelfNpc,
		UDamageType::StaticClass());

#if !UE_BUILD_SHIPPING
	DrawDebugLine(GetWorld(), Start, Hit.ImpactPoint, FColor::Orange, false, 0.2f, 0, 1.5f);
#endif
}
