#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHousePlacedObjectSettingsWidget.generated.h"

class ATheHouseObject;

/**
 * WBP de paramètres d’un objet casino (RTS), assigné sur le PlayerController (PlacedObjectSettingsWidgetClass).
 * Ne s’ouvre que si ATheHouseObject::bOpenParametersPanelOnLeftClick est coché sur le BP de l’objet.
 * Dans « After Target Placed Object Changed », lire GetTargetPlacedObject() (bNpcMoneyFlowEnabled, NpcSpendToPlay, …).
 * Fermeture : Échap (cascade du PC), bouton qui appelle ClosePlacedObjectSettingsPanel sur le PC, ou désélection.
 */
UCLASS(Abstract, Blueprintable)
class THEHOUSE_API UTheHousePlacedObjectSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
	void SetTargetPlacedObject(ATheHouseObject* Obj);

	UFUNCTION(BlueprintPure, Category="TheHouse|RTS|UI")
	ATheHouseObject* GetTargetPlacedObject() const { return TargetObject.Get(); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category="TheHouse|RTS|UI", meta=(DisplayName="After Target Placed Object Changed"))
	void BP_OnTargetPlacedObjectChanged();

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> TargetObject = nullptr;
};
