#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "TheHouseLanguageSaveGame.generated.h"

/** Sauvegarde du choix de langue (slot dédié, hors options graphiques). */
UCLASS()
class THEHOUSE_API UTheHouseLanguageSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Identifiant culture UE (ex. fr, en, en-US). */
	UPROPERTY(VisibleAnywhere, Category = "TheHouse|Localization")
	FString PreferredCultureName;
};
