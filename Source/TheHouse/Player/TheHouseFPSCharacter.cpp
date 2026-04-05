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

	// Set size and collision for capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	// Create a CameraComponent	
	FPSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FPSCameraComponent->SetupAttachment(GetCapsuleComponent());
	FPSCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FPSCameraComponent->bUsePawnControlRotation = true;

	// Default movement settings (can be tweaked in BP)
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->GravityScale = 1.0f;          // Gravité normale
	GetCharacterMovement()->DefaultLandMovementMode = MOVE_Walking; // Jamais Flying
	
	// AirControl : permet de se déplacer (ZQSD) pendant la chute si spawn en l'air
	GetCharacterMovement()->AirControl = 0.5f;

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

// void ATheHouseFPSCharacter::BeginPlay()
// {
// 	Super::BeginPlay();

// 	UCharacterMovementComponent* MoveComp = GetCharacterMovement();

// 	MoveComp->SetMovementMode(MOVE_Walking);
// 	MoveComp->bCheatFlying = false;
// 	MoveComp->GravityScale = 1.f;

// 	// Sécurité collision capsule
// 	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
// 	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));
// }

// Called every frame
void ATheHouseFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    float Fwd = 0.f;
    float Rt = 0.f;

    if (bMoveFwd) Fwd += 1.f;
    if (bMoveBwd) Fwd -= 1.f;
    if (bMoveRight) Rt += 1.f;
    if (bMoveLeft) Rt -= 1.f;

    if (Fwd != 0.f)
    {
        AddMovementInput(GetActorForwardVector(), Fwd);
    }
    if (Rt != 0.f)
    {
        AddMovementInput(GetActorRightVector(), Rt);
    }
}

// Called to bind functionality to input
void ATheHouseFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

    // BIND DIRECT DES TOUCHES MATERIELLES (Bypass tous les bugs d'axe / GameOnly / EnhancedInput)
    // PlayerInputComponent->BindKey(EKeys::W, IE_Pressed, this, &ATheHouseFPSCharacter::MoveFwdPressed);
    // PlayerInputComponent->BindKey(EKeys::W, IE_Released, this, &ATheHouseFPSCharacter::MoveFwdReleased);
    // PlayerInputComponent->BindKey(EKeys::Z, IE_Pressed, this, &ATheHouseFPSCharacter::MoveFwdPressed);
    // PlayerInputComponent->BindKey(EKeys::Z, IE_Released, this, &ATheHouseFPSCharacter::MoveFwdReleased);
    // PlayerInputComponent->BindKey(EKeys::S, IE_Pressed, this, &ATheHouseFPSCharacter::MoveBwdPressed);
    // PlayerInputComponent->BindKey(EKeys::S, IE_Released, this, &ATheHouseFPSCharacter::MoveBwdReleased);
    
    // PlayerInputComponent->BindKey(EKeys::D, IE_Pressed, this, &ATheHouseFPSCharacter::MoveRightPressed);
    // PlayerInputComponent->BindKey(EKeys::D, IE_Released, this, &ATheHouseFPSCharacter::MoveRightReleased);
    // PlayerInputComponent->BindKey(EKeys::A, IE_Pressed, this, &ATheHouseFPSCharacter::MoveLeftPressed);
    // PlayerInputComponent->BindKey(EKeys::A, IE_Released, this, &ATheHouseFPSCharacter::MoveLeftReleased);
    // PlayerInputComponent->BindKey(EKeys::Q, IE_Pressed, this, &ATheHouseFPSCharacter::MoveLeftPressed);
    // PlayerInputComponent->BindKey(EKeys::Q, IE_Released, this, &ATheHouseFPSCharacter::MoveLeftReleased);
}

void ATheHouseFPSCharacter::MoveFwdPressed() { bMoveFwd = true; }
void ATheHouseFPSCharacter::MoveFwdReleased() { bMoveFwd = false; }
void ATheHouseFPSCharacter::MoveBwdPressed() { bMoveBwd = true; }
void ATheHouseFPSCharacter::MoveBwdReleased() { bMoveBwd = false; }
void ATheHouseFPSCharacter::MoveRightPressed() { bMoveRight = true; }
void ATheHouseFPSCharacter::MoveRightReleased() { bMoveRight = false; }
void ATheHouseFPSCharacter::MoveLeftPressed() { bMoveLeft = true; }
void ATheHouseFPSCharacter::MoveLeftReleased() { bMoveLeft = false; }

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
