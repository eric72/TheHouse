#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseNPCRTSContextMenuUMGWidget.generated.h"

class ATheHouseNPCCharacter;
class ATheHousePlayerController;
class UBorder;
class UVerticalBox;
class UTheHouseRTSUIClickRelay;

/**
 * Menu contextuel RTS pour les PNJ (ATheHouseNPCCharacter), séparé du menu des objets placés.
 *
 * Dérivez un Widget Blueprint de cette classe pour personnaliser le layout (BindWidgetOptional :
 * MenuBorder, OptionsBox comme UTheHouseRTSContextMenuUMGWidget). Les boutons sont générés depuis
 * `ATheHousePlayerController::NPCRTSContextMenuOptionDefs` ; les ids sont traités dans
 * `ExecuteNPCRTSContextMenuOption` / `HandleNPCRTSContextMenuOption`.
 */
UCLASS(Blueprintable)
class THEHOUSE_API UTheHouseNPCRTSContextMenuUMGWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI|NPC")
	void OpenForNPC(ATheHousePlayerController* InOwnerPC, ATheHouseNPCCharacter* InTargetNPC, const FVector2D& ScreenPosition);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UBorder> MenuBorder = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> OptionsBox = nullptr;

private:
	void EnsureRuntimeOptionsBox();
	void RebuildOptionButtons();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseNPCCharacter> TargetNPC = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTheHouseRTSUIClickRelay>> ClickRelays;
};
