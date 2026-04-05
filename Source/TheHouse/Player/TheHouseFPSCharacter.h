// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "TheHouse/Core/TheHouseSelectable.h"
#include "TheHouseFPSCharacter.generated.h"

UCLASS()
class THEHOUSE_API ATheHouseFPSCharacter : public ACharacter, public ITheHouseSelectable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ATheHouseFPSCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// FPS Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* FPSCameraComponent;

	// --- ITheHouseSelectable Interface ---
	virtual void OnSelect() override;
	virtual void OnDeselect() override;
	virtual bool IsSelectable() const override;

	/** If false, this character cannot be selected by the RTS Controller */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	bool bCanBeSelected;

private:
    bool bMoveFwd = false;
    bool bMoveBwd = false;
    bool bMoveRight = false;
    bool bMoveLeft = false;

    void MoveFwdPressed();
    void MoveFwdReleased();
    void MoveBwdPressed();
    void MoveBwdReleased();
    void MoveRightPressed();
    void MoveRightReleased();
    void MoveLeftPressed();
    void MoveLeftReleased();
};
