// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "TheHouse/Core/TheHouseSelectable.h"
#include "TheHouseFPSCharacter.generated.h"

class UTheHouseHealthComponent;
class ATheHouseWeapon;

UCLASS()
class THEHOUSE_API ATheHouseFPSCharacter : public ACharacter, public ITheHouseSelectable
{
	GENERATED_BODY()

public:
	ATheHouseFPSCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	UCameraComponent* FPSCameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Health")
	TObjectPtr<UTheHouseHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="TheHouse|Combat")
	TObjectPtr<ATheHouseWeapon> EquippedWeapon;

	/** Weapon to spawn on BeginPlay (BP-friendly). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat")
	TSubclassOf<ATheHouseWeapon> DefaultWeaponClass;

	/** Socket (sur GetMesh()) où attacher l'arme. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|Combat")
	FName WeaponAttachSocketName = FName(TEXT("hand_rSocket"));

	UFUNCTION(BlueprintCallable, Category="TheHouse|Combat")
	void FireWeaponOnce();

	// --- ITheHouseSelectable ---
	virtual void OnSelect() override;
	virtual void OnDeselect() override;
	virtual bool IsSelectable() const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Selection")
	bool bCanBeSelected;

	/**
	 * When true (default), all primitive components except the capsule have collision disabled after spawn.
	 * Tag a component "KeepMeshCollisionInFPS" to leave its collision unchanged.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Collision")
	bool bUseCapsuleOnlyForWorldCollision = true;

	UFUNCTION(BlueprintCallable, Category = "TheHouse|FPS|Collision")
	void ApplyFirstPersonCapsuleOnlyCollision();

	/** Marche (vitesse de base, hors sprint / accroupi). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Movement", meta = (ClampMin = "0.0"))
	float WalkSpeed = 600.f;

	/** Vitesse avec action « courir » maintenue (voir DefaultInput.ini : TheHouseFPSRun). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Movement", meta = (ClampMin = "0.0"))
	float SprintWalkSpeed = 960.f;

	/** Vitesse max en position accroupie (TheHouseFPS_Crouch). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Movement", meta = (ClampMin = "0.0"))
	float CrouchedMoveSpeed = 280.f;

	UFUNCTION(BlueprintCallable, Category = "TheHouse|FPS|Movement")
	void SetFPSMovementSprinting(bool bSprint);

	UFUNCTION(BlueprintPure, Category = "TheHouse|FPS|Movement")
	bool IsFPSMovementSprinting() const { return bWantsSprint; }

	/**
	 * Si false : caméra sur la capsule (offset fixe, ne suit pas les animations de tête).
	 * Si true : caméra attachée au socket du mesh (tête / yeux) — suit le squelette (bob, regard anim, etc.).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera")
	bool bCameraFollowsCharacterHead = false;

	/** Socket sur GetMesh() — doit correspondre au nom dans le Skeleton/Mesh (ex. HeadCameraEmplacement, head, Head). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera", meta = (EditCondition = "bCameraFollowsCharacterHead"))
	FName FPSHeadCameraSocketName = FName(TEXT("HeadCameraEmplacement"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera", meta = (EditCondition = "bCameraFollowsCharacterHead"))
	FVector FPSCameraSocketRelativeLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera", meta = (EditCondition = "bCameraFollowsCharacterHead"))
	FRotator FPSCameraSocketRelativeRotation = FRotator::ZeroRotator;

	/** Offset caméra quand bCameraFollowsCharacterHead est false (parent = capsule). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera")
	FVector FPSCameraCapsuleRelativeLocation = FVector(-10.f, 0.f, 60.f);

	/**
	 * FPS: si true, applique une stratégie anti-clipping pour le joueur local quand la caméra est sur la tête.
	 * Par défaut on cache seulement la tête/nuque (le corps reste visible).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera")
	bool bHideMeshForOwnerInFPS = true;

	/** Si true (défaut), cache uniquement certains os (tête/nuque) pour le joueur local. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera", meta=(EditCondition="bHideMeshForOwnerInFPS"))
	bool bHideOnlyHeadBonesForOwnerInFPS = true;

	/** Si true, cache tout le mesh pour le joueur local (fallback si tu veux un FPS « arms-only » plus tard). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera", meta=(EditCondition="bHideMeshForOwnerInFPS"))
	bool bHideFullBodyMeshForOwnerInFPS = false;

	/** Liste d'os à cacher pour le joueur local quand bHideOnlyHeadBonesForOwnerInFPS=true. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|Camera", meta=(EditCondition="bHideMeshForOwnerInFPS"))
	TArray<FName> OwnerHiddenBonesInFPS = { FName(TEXT("Head")), FName(TEXT("Neck")) };

	UFUNCTION(BlueprintCallable, Category = "TheHouse|FPS|Camera")
	void RefreshFPSCameraAttachment();

	/**
	 * Regard « libre » : la caméra suit le contrôleur (souris) ; le corps (capsule + mesh) ne pivote pas tout de suite.
	 * Tant que |yaw contrôleur − yaw acteur| &lt; seuil, tu regardes sur le côté sans tourner les pieds au monde.
	 * Au-delà du seuil, l’acteur pivote en yaw pour rattraper la visée (vitesse limitée par BodyYawAlignDegreesPerSecond).
	 * Pieds parfaitement plantés sans glisser = animations « turn in place » / root motion dans l’Anim BP (optionnel).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw")
	bool bDelayBodyYawUntilLookSideways = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw", meta = (EditCondition = "bDelayBodyYawUntilLookSideways", ClampMin = "10.0", ClampMax = "170.0"))
	float BodyYawVsControlThresholdDegrees = 70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw", meta = (EditCondition = "bDelayBodyYawUntilLookSideways", ClampMin = "1.0"))
	float BodyYawAlignDegreesPerSecond = 120.f;

	/**
	 * Quand le joueur se déplace, on réaligne le corps vers la visée pour éviter l'effet « corps de côté ».
	 * (le free-look reste utile à l'arrêt, mais en mouvement on veut une silhouette cohérente).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw", meta = (EditCondition = "bDelayBodyYawUntilLookSideways"))
	bool bAlignBodyYawToControlWhenMoving = true;

	/** Seuil de vitesse 2D (uu/s) au-dessus duquel on considère « en mouvement » pour réaligner le corps. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw", meta = (EditCondition = "bDelayBodyYawUntilLookSideways", ClampMin = "0.0"))
	float BodyYawAlignMoveSpeedThreshold = 12.f;

	/** Vitesse de réalignement (deg/s) utilisée quand on est en mouvement. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw", meta = (EditCondition = "bDelayBodyYawUntilLookSideways", ClampMin = "1.0"))
	float BodyYawAlignMovingDegreesPerSecond = 540.f;

	/** Si true, le pivot retardé du corps ne s’applique qu’au sol (évite spin en l’air). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TheHouse|FPS|BodyYaw", meta = (EditCondition = "bDelayBodyYawUntilLookSideways"))
	bool bBodyYawAlignOnlyOnGround = true;

	/** Applique bUseControllerRotationYaw / orient movement selon bDelayBodyYawUntilLookSideways (rappeler si tu changes la propriété en runtime). */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|FPS|BodyYaw")
	void ApplyFreeLookBodyYawMode();

private:
	bool bWantsSprint = false;

	void ApplyFPSMovementSpeeds();
	void UpdateBodyYawTowardControl(float DeltaTime);
};
