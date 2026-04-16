#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "TheHouseObjectSlotUserComponent.generated.h"

class AAIController;
class ATheHouseObject;
class UAnimMontage;

UENUM(BlueprintType)
enum class EObjectSlotUseState : uint8
{
	Idle,
	MovingToSlot,
	Aligned,
	PlayingMontage
};

/**
 * Simple "premium" object-usage helper:
 * - reserve a slot on an ATheHouseObject
 * - MoveTo near the socket
 * - snap/align to the socket transform
 * - optionally play a montage
 *
 * Works from Blueprint without needing a Behavior Tree.
 */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class THEHOUSE_API UTheHouseObjectSlotUserComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTheHouseObjectSlotUserComponent();

	/** Use a slot on an object. Returns false if no slot was available. */
	UFUNCTION(BlueprintCallable, Category="Casino|NPC")
	bool UseObjectSlot(ATheHouseObject* TargetObject, FName PurposeTag, UAnimMontage* MontageToPlay);

	/** Stop using the current slot (releases it). */
	UFUNCTION(BlueprintCallable, Category="Casino|NPC")
	void StopUsingSlot();

	UFUNCTION(BlueprintPure, Category="Casino|NPC")
	EObjectSlotUseState GetState() const { return State; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> CurrentObject = nullptr;

	UPROPERTY(Transient)
	int32 CurrentSlotIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FTransform CurrentSlotTransform;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> PendingMontage = nullptr;

	UPROPERTY(Transient)
	EObjectSlotUseState State = EObjectSlotUseState::Idle;

	UPROPERTY(EditAnywhere, Category="Casino|NPC", meta=(ClampMin="0.0"))
	float AcceptableRadius = 35.f;

	UPROPERTY(EditAnywhere, Category="Casino|NPC", meta=(ClampMin="0.0"))
	float AlignInterpSpeed = 14.f;

	void TickAlign(float DeltaTime);

	/** Signature alignée sur AAIController::ReceiveMoveCompleted (UE 5.7 : EPathFollowingResult::Type). */
	UFUNCTION()
	void OnOwnerMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);

	AAIController* GetAIController() const;
	APawn* GetPawnOwner() const;
};

