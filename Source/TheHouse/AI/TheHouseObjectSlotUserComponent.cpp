#include "AI/TheHouseObjectSlotUserComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "TheHouseObject.h"

UTheHouseObjectSlotUserComponent::UTheHouseObjectSlotUserComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTheHouseObjectSlotUserComponent::BeginPlay()
{
	Super::BeginPlay();
}

APawn* UTheHouseObjectSlotUserComponent::GetPawnOwner() const
{
	return Cast<APawn>(GetOwner());
}

AAIController* UTheHouseObjectSlotUserComponent::GetAIController() const
{
	APawn* P = GetPawnOwner();
	if (!P)
	{
		return nullptr;
	}
	return Cast<AAIController>(P->GetController());
}

bool UTheHouseObjectSlotUserComponent::UseObjectSlot(ATheHouseObject* TargetObject, FName PurposeTag, UAnimMontage* MontageToPlay)
{
	StopUsingSlot();

	APawn* P = GetPawnOwner();
	if (!P || !TargetObject)
	{
		return false;
	}

	int32 SlotIndex = INDEX_NONE;
	FTransform SlotT;
	if (!TargetObject->TryReserveNPCSlot(P, PurposeTag, SlotIndex, SlotT))
	{
		return false;
	}

	CurrentObject = TargetObject;
	CurrentSlotIndex = SlotIndex;
	CurrentSlotTransform = SlotT;
	PendingMontage = MontageToPlay;
	State = EObjectSlotUseState::MovingToSlot;

	AAIController* AI = GetAIController();
	if (!AI)
	{
		// No AI controller: snap immediately.
		P->SetActorLocationAndRotation(SlotT.GetLocation(), SlotT.Rotator());
		State = EObjectSlotUseState::Aligned;
		return true;
	}

	AI->ReceiveMoveCompleted.AddDynamic(this, &UTheHouseObjectSlotUserComponent::OnOwnerMoveCompleted);

	// Navigate to the slot location (or closest point).
	const FVector Goal = SlotT.GetLocation();
	FAIMoveRequest Req;
	Req.SetGoalLocation(Goal);
	Req.SetAcceptanceRadius(AcceptableRadius);
	Req.SetUsePathfinding(true);
	Req.SetAllowPartialPath(true);

	AI->MoveTo(Req);
	return true;
}

void UTheHouseObjectSlotUserComponent::StopUsingSlot()
{
	AAIController* AI = GetAIController();
	if (AI)
	{
		AI->StopMovement();
		AI->ReceiveMoveCompleted.RemoveDynamic(this, &UTheHouseObjectSlotUserComponent::OnOwnerMoveCompleted);
	}

	APawn* P = GetPawnOwner();
	if (CurrentObject && CurrentSlotIndex != INDEX_NONE)
	{
		CurrentObject->ReleaseNPCSlotByIndex(CurrentSlotIndex, P);
	}

	CurrentObject = nullptr;
	CurrentSlotIndex = INDEX_NONE;
	PendingMontage = nullptr;
	State = EObjectSlotUseState::Idle;
}

void UTheHouseObjectSlotUserComponent::OnOwnerMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result)
{
	(void)RequestID;
	(void)Result;

	AAIController* AI = GetAIController();
	if (AI)
	{
		AI->ReceiveMoveCompleted.RemoveDynamic(this, &UTheHouseObjectSlotUserComponent::OnOwnerMoveCompleted);
	}

	if (!CurrentObject || CurrentSlotIndex == INDEX_NONE)
	{
		State = EObjectSlotUseState::Idle;
		return;
	}

	// Refresh transform from socket (object may have moved).
	FTransform SlotT;
	if (CurrentObject->GetNPCSlotWorldTransform(CurrentSlotIndex, SlotT))
	{
		CurrentSlotTransform = SlotT;
	}

	State = EObjectSlotUseState::Aligned;
}

void UTheHouseObjectSlotUserComponent::TickAlign(float DeltaTime)
{
	APawn* P = GetPawnOwner();
	if (!P)
	{
		return;
	}

	const FVector TargetLoc = CurrentSlotTransform.GetLocation();
	const FRotator TargetRot = CurrentSlotTransform.Rotator();

	const FVector NewLoc = FMath::VInterpTo(P->GetActorLocation(), TargetLoc, DeltaTime, AlignInterpSpeed);
	const FRotator NewRot = FMath::RInterpTo(P->GetActorRotation(), TargetRot, DeltaTime, AlignInterpSpeed);
	P->SetActorLocationAndRotation(NewLoc, NewRot);
}

void UTheHouseObjectSlotUserComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (State == EObjectSlotUseState::Aligned)
	{
		TickAlign(DeltaTime);

		// Play montage once after initial align.
		if (PendingMontage)
		{
			if (ACharacter* C = Cast<ACharacter>(GetOwner()))
			{
				if (UAnimInstance* Anim = C->GetMesh() ? C->GetMesh()->GetAnimInstance() : nullptr)
				{
					Anim->Montage_Play(PendingMontage);
					State = EObjectSlotUseState::PlayingMontage;
					PendingMontage = nullptr;
				}
			}
		}
	}
}

