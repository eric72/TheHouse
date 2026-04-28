// Fill out your copyright notice in the Description page of Project Settings.

#include "TheHouseFPSCharacter.h"
#include "TheHouse/Combat/TheHouseWeapon.h"
#include "TheHouse/Core/TheHouseHealthComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

ATheHouseFPSCharacter::ATheHouseFPSCharacter()
{
	OverrideInputComponentClass = UInputComponent::StaticClass();

	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(UCollisionProfile::Pawn_ProfileName);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	FPSCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FPSCameraComponent->SetupAttachment(GetCapsuleComponent());
	FPSCameraComponent->SetRelativeLocation(FPSCameraCapsuleRelativeLocation);
	FPSCameraComponent->bUsePawnControlRotation = true;

	HealthComponent = CreateDefaultSubobject<UTheHouseHealthComponent>(TEXT("HealthComponent"));

	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = WalkSpeed;
		Move->MaxWalkSpeedCrouched = CrouchedMoveSpeed;
		Move->JumpZVelocity = 600.f;
		Move->GravityScale = 1.0f;
		Move->DefaultLandMovementMode = MOVE_Walking;
		Move->AirControl = 0.5f;
		Move->GetNavAgentPropertiesRef().bCanCrouch = true;
		Move->SetCrouchedHalfHeight(50.f);
	}

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	bCanBeSelected = true;
}

void ATheHouseFPSCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplyFirstPersonCapsuleOnlyCollision();
	RefreshFPSCameraAttachment();
	ApplyFPSMovementSpeeds();
	ApplyFreeLookBodyYawMode();

	// Spawn weapon (optional).
	if (DefaultWeaponClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		EquippedWeapon = GetWorld()->SpawnActor<ATheHouseWeapon>(DefaultWeaponClass, Params);
		if (EquippedWeapon && GetMesh())
		{
			EquippedWeapon->AttachToComponent(
				GetMesh(),
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				WeaponAttachSocketName);
		}
	}
}

void ATheHouseFPSCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	// ACharacter::Restart() (depuis Possess) peut remettre bUseControllerRotationYaw.
	ApplyFreeLookBodyYawMode();
	RefreshFPSCameraAttachment(); // applique aussi OwnerNoSee si besoin
}

void ATheHouseFPSCharacter::ApplyFirstPersonCapsuleOnlyCollision()
{
	if (!bUseCapsuleOnlyForWorldCollision)
	{
		return;
	}

	TArray<UPrimitiveComponent*> Primitives;
	GetComponents<UPrimitiveComponent>(Primitives);
	UCapsuleComponent* Cap = GetCapsuleComponent();
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim || Prim == Cap)
		{
			continue;
		}
		if (Prim->ComponentHasTag(TEXT("KeepMeshCollisionInFPS")))
		{
			continue;
		}
		Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Prim->SetGenerateOverlapEvents(false);
	}
}

static FName TheHouse_ResolveHeadCameraSocket(USkeletalMeshComponent* Skel, FName Preferred)
{
	if (!Skel)
	{
		return NAME_None;
	}
	if (!Preferred.IsNone() && Skel->DoesSocketExist(Preferred))
	{
		return Preferred;
	}
	static const FName FallbackNames[] = {
		FName(TEXT("HeadCameraEmplacement")),
		FName(TEXT("head")),
		FName(TEXT("Head")),
		FName(TEXT("eyes")),
		FName(TEXT("EyeSocket")),
	};
	for (const FName& Candidate : FallbackNames)
	{
		if (!Candidate.IsNone() && Skel->DoesSocketExist(Candidate))
		{
			return Candidate;
		}
	}
	return NAME_None;
}

void ATheHouseFPSCharacter::RefreshFPSCameraAttachment()
{
	if (!FPSCameraComponent)
	{
		return;
	}

	USkeletalMeshComponent* Skel = GetMesh();
	const FName SocketToUse =
		bCameraFollowsCharacterHead ? TheHouse_ResolveHeadCameraSocket(Skel, FPSHeadCameraSocketName) : NAME_None;

	// FPS : anti-clipping (joueur local) — idéalement on cache seulement la tête/nuque, pas tout le corps.
	if (Skel)
	{
		const bool bLocal = IsLocallyControlled();
		const bool bApplyOwnerHiding = bHideMeshForOwnerInFPS && bLocal && bCameraFollowsCharacterHead;

		Skel->SetOwnerNoSee(bApplyOwnerHiding && bHideFullBodyMeshForOwnerInFPS);

		// Reset d'abord : si on quitte le FPS ou si on toggle le bool en runtime, il faut réafficher.
		if (!bApplyOwnerHiding || !bHideOnlyHeadBonesForOwnerInFPS)
		{
			for (const FName& Bone : OwnerHiddenBonesInFPS)
			{
				if (!Bone.IsNone())
				{
					Skel->UnHideBoneByName(Bone);
				}
			}
		}
		else
		{
			for (const FName& Bone : OwnerHiddenBonesInFPS)
			{
				if (!Bone.IsNone())
				{
					Skel->HideBoneByName(Bone, EPhysBodyOp::PBO_None);
				}
			}
		}
	}

	if (bCameraFollowsCharacterHead && Skel && !SocketToUse.IsNone())
	{
		// En jeu, préférer AttachToComponent : SetupAttachment seul après spawn peut ne pas
		// prendre correctement le socket sur le mesh (caméra « au milieu » du corps).
		FPSCameraComponent->AttachToComponent(
			Skel,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			SocketToUse);
		FPSCameraComponent->SetRelativeLocation(FPSCameraSocketRelativeLocation);
		FPSCameraComponent->SetRelativeRotation(FPSCameraSocketRelativeRotation);
	}
	else
	{
		if (bCameraFollowsCharacterHead && GetWorld() && GetWorld()->IsGameWorld())
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[TheHouseFPS] Pas de socket tête utilisable (demandé '%s') sur %s — vérifie le nom sur le **Skeleton** et GetMesh(). Retombée sur capsule."),
				FPSHeadCameraSocketName.IsNone() ? TEXT("(aucun)") : *FPSHeadCameraSocketName.ToString(),
				Skel ? *Skel->GetName() : TEXT("(pas de mesh)"));
		}
		FPSCameraComponent->AttachToComponent(
			GetCapsuleComponent(),
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			NAME_None);
		FPSCameraComponent->SetRelativeLocation(FPSCameraCapsuleRelativeLocation);
		FPSCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);
	}

	FPSCameraComponent->bUsePawnControlRotation = true;
}

void ATheHouseFPSCharacter::SetFPSMovementSprinting(bool bSprint)
{
	bWantsSprint = bSprint;
	ApplyFPSMovementSpeeds();
}

void ATheHouseFPSCharacter::ApplyFPSMovementSpeeds()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	Move->MaxWalkSpeedCrouched = CrouchedMoveSpeed;
	if (IsCrouched())
	{
		return;
	}

	Move->MaxWalkSpeed = bWantsSprint ? SprintWalkSpeed : WalkSpeed;
}

void ATheHouseFPSCharacter::ApplyFreeLookBodyYawMode()
{
	if (bDelayBodyYawUntilLookSideways)
	{
		bUseControllerRotationYaw = false;
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->bOrientRotationToMovement = false;
		}
	}
	else
	{
		bUseControllerRotationYaw = true;
	}
}

void ATheHouseFPSCharacter::UpdateBodyYawTowardControl(float DeltaTime)
{
	if (!bDelayBodyYawUntilLookSideways || !IsLocallyControlled())
	{
		return;
	}

	AController* C = GetController();
	if (!C)
	{
		return;
	}

	if (bBodyYawAlignOnlyOnGround)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			if (!Move->IsMovingOnGround())
			{
				return;
			}
		}
	}

	const float TargetYaw = C->GetControlRotation().Yaw;
	const float ActorYaw = GetActorRotation().Yaw;
	const float Delta = FMath::FindDeltaAngleDegrees(ActorYaw, TargetYaw);

	float EffectiveThreshold = BodyYawVsControlThresholdDegrees;
	float EffectiveAlignRate = BodyYawAlignDegreesPerSecond;

	// En mouvement : réaligner pour éviter la silhouette "tordue" (corps de côté mais déplacement vers la visée).
	if (bAlignBodyYawToControlWhenMoving)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			const float Speed2D = FVector(Move->Velocity.X, Move->Velocity.Y, 0.f).Size();
			if (Speed2D > FMath::Max(0.f, BodyYawAlignMoveSpeedThreshold))
			{
				EffectiveThreshold = 0.f;
				EffectiveAlignRate = BodyYawAlignMovingDegreesPerSecond;
			}
		}
	}

	if (FMath::Abs(Delta) <= EffectiveThreshold)
	{
		return;
	}

	const float MaxStep = FMath::Max(1.f, EffectiveAlignRate) * DeltaTime;
	const float Step = FMath::Clamp(Delta, -MaxStep, MaxStep);
	AddActorWorldRotation(FRotator(0.f, Step, 0.f));
}

void ATheHouseFPSCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ApplyFPSMovementSpeeds();
	UpdateBodyYawTowardControl(DeltaTime);
}

void ATheHouseFPSCharacter::FireWeaponOnce()
{
	if (!EquippedWeapon)
	{
		return;
	}
	AController* C = GetController();
	const FVector Dir = C ? C->GetControlRotation().Vector() : GetActorForwardVector();
	EquippedWeapon->FireInDirection(Dir);
}

void ATheHouseFPSCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ATheHouseFPSCharacter::OnSelect()
{
	UE_LOG(LogTemp, Warning, TEXT("Character Selected: %s"), *GetName());

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetRenderCustomDepth(true);
	}
}

void ATheHouseFPSCharacter::OnDeselect()
{
	UE_LOG(LogTemp, Warning, TEXT("Character Deselected: %s"), *GetName());

	if (USkeletalMeshComponent* SkelMesh = GetMesh())
	{
		SkelMesh->SetRenderCustomDepth(false);
	}
}

bool ATheHouseFPSCharacter::IsSelectable() const
{
	return bCanBeSelected;
}
