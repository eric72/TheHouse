#pragma once

#include "CoreMinimal.h"
#include "TheHouseObject.h"
#include "TheHouseRTSUITypes.generated.h"

/** Identifiants d’options intégrées (menu contextuel RTS). */
namespace TheHouseRTSContextIds
{
	inline const FName DeleteObject(TEXT("Delete"));
	inline const FName StoreObject(TEXT("Store"));
}

/** Entrée du catalogue de pose (palette RTS). */
USTRUCT(BlueprintType)
struct FTheHousePlaceableCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	TSubclassOf<ATheHouseObject> ObjectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	FText DisplayName;
};

/** Définition d’une option de menu contextuel (modifiable dans le BP du PlayerController). */
USTRUCT(BlueprintType)
struct FTheHouseRTSContextMenuOptionDef
{
	GENERATED_BODY()

	/** Identifiant stable : Delete / Store pour les actions intégrées, ou valeur libre pour HandleRTSContextMenuOption. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	FName OptionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS")
	FText Label;
};
