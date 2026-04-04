// Fill out your copyright notice in the Description page of Project Settings.

#include "TheHouseFPSCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"

// Sets default values
ATheHouseFPSCharacter::ATheHouseFPSCharacter()
{
	OverrideInputComponentClass = UInputComponent::StaticClass();

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FPSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FPSCameraComponent->SetupAttachment(GetCapsuleComponent());
	FPSCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FPSCameraComponent->bUsePawnControlRotation = true;

	// Default movement settings (can be tweaked in BP)
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->JumpZVelocity = 600.f;

	// Rotation Handling:
	// Let the controller rotate the Yaw (turning)
	bUseControllerRotationPitch = false; // Don't pitch the body
	bUseControllerRotationYaw = true;    // Turn the body
	bUseControllerRotationRoll = false;
	
	// Default Selection State
	bCanBeSelected = true;
}

// Called when the game starts or when spawned
void ATheHouseFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ATheHouseFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

// Called to bind functionality to input
void ATheHouseFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Input bindings are handled by PlayerController technically, 
	// but standard Character functions (Jump, Move) are often bound here or in PC.
	// For this hybrid system, we will try to centralize logic in PlayerController 
	// or delegate back to here. 
	// We'll leave this empty for now and let PC manage inputs to avoid duplication.
}

void ATheHouseFPSCharacter::OnSelect()
{
	UE_LOG(LogTemp, Warning, TEXT("Character Selected: %s"), *GetName());
	
	// Visual Feedback: Show Outline (Custom Depth)
	GetMesh()->SetRenderCustomDepth(true);
}

void ATheHouseFPSCharacter::OnDeselect()
{
	UE_LOG(LogTemp, Warning, TEXT("Character Deselected: %s"), *GetName());

	// Remove Visual Feedback
	GetMesh()->SetRenderCustomDepth(false);
}

bool ATheHouseFPSCharacter::IsSelectable() const
{
	return bCanBeSelected;
}
