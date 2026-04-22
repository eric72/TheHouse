// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCArchetype.h"
#include "TheHouseNPCStaffArchetype.generated.h"

class UTexture2D;
class USkeletalMesh;

/**
 * Archetype réservé au personnel du casino.
 * Pas de « prix d’entrée » : ce concept est cantonné aux archetypes Client si vous l’activez plus tard.
 *
 * DefaultMonthlySalary initialise le salaire sur le personnage ; le joueur peut le modifier en runtime
 * via StaffMonthlySalary sur ATheHouseNPCCharacter (UI RH).
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseNPCStaffArchetype : public UTheHouseNPCArchetype
{
	GENERATED_BODY()

public:
	UTheHouseNPCStaffArchetype();

	/**
	 * Libellé métier pour BT / C++ / UI (ex. DEALER, GUARD) : ce n’est pas un GUID moteur auto-généré.
	 * À renseigner à la main sur le Data Asset (ou laisser vide et n’utiliser que le type d’archetype / le nom du fichier en convention équipe).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Role")
	FName StaffRoleId = NAME_None;

	/** Salaire mensuel par défaut à l’embauche / au spawn (copié vers le personnage). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Economy", meta = (ClampMin = "0.0"))
	float DefaultMonthlySalary = 0.f;

	/**
	 * Niveau initial (1–5 étoiles) : performance / revenus potentiels vs coût — à relier à votre économie.
	 * Copié vers StaffStarRating sur le personnage au spawn.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Progression", meta = (ClampMin = "1", ClampMax = "5"))
	int32 DefaultStarRating = 3;

	/**
	 * Si défini, remplace l’Animation Blueprint du mesh à l’application de l’archetype (garde vs croupier vs portier).
	 * Doit rester compatible avec le Skeleton du mesh utilisé.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Animation")
	TSubclassOf<UAnimInstance> AnimationBlueprintOverride;

	/**
	 * Variantes de silhouette pour le même métier (ex. plusieurs croupiers). Au BeginPlay, si la liste n’est pas vide,
	 * un mesh est tiré au hasard et appliqué au composant Mesh du personnage.
	 * Toutes les entrées doivent partager le même Skeleton que l’Animation Blueprint du Blueprint PNJ.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Visual")
	TArray<TObjectPtr<USkeletalMesh>> SkeletalMeshVariants;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Presentation")
	TObjectPtr<UTexture2D> StaffPortrait;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NPC|Archetype|Staff|Presentation")
	FLinearColor UiAccentColor = FLinearColor::White;
};
