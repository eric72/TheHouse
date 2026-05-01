#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseSettingsMenuWidget.generated.h"

class ATheHousePlayerController;
class UTheHouseGameUserSettings;

/**
 * Racine UI pour Paramètres (graphismes, sons, touches). Créer un Widget Blueprint
 * qui hérite de cette classe et brancher les contrôles sur UTheHouseGameUserSettings.
 */
UCLASS(Abstract, Blueprintable)
class THEHOUSE_API UTheHouseSettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TheHouse|UI|Settings")
	void BindToPlayerController(ATheHousePlayerController* PC);

	UFUNCTION(BlueprintPure, Category = "TheHouse|UI|Settings")
	ATheHousePlayerController* GetTheHouseOwnerPC() const { return OwnerPC.Get(); }

	UFUNCTION(BlueprintPure, Category = "TheHouse|UI|Settings")
	UTheHouseGameUserSettings* GetTheHouseGameUserSettings() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "TheHouse|UI|Settings", meta = (DisplayName = "On Settings Menu Constructed"))
	void BP_OnSettingsMenuConstructed();

	UPROPERTY(Transient)
	TWeakObjectPtr<ATheHousePlayerController> OwnerPC;
};
