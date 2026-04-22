#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseRTSContextMenuUMGWidget.generated.h"

class ATheHouseObject;
class ATheHousePlayerController;
class UBorder;
class UVerticalBox;
class UTheHouseRTSUIClickRelay;

/**
 * Variante UMG du menu contextuel RTS (BP-friendly).
 * - Le rendu/layout se fait dans un Widget Blueprint dérivé (BindWidgetOptional).
 * - Les options sont générées dynamiquement (boutons).
 */
UCLASS(Blueprintable)
class THEHOUSE_API UTheHouseRTSContextMenuUMGWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="TheHouse|RTS|UI")
	void OpenForTarget(ATheHousePlayerController* InOwnerPC, ATheHouseObject* InTarget, const FVector2D& ScreenPosition);

protected:
	virtual void NativeConstruct() override;

	/** Racine visuelle (optionnelle). */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UBorder> MenuBorder = nullptr;

	/** Conteneur des options (optionnel). */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UVerticalBox> OptionsBox = nullptr;

private:
	/** Si le Widget Blueprint n’a pas de VerticalBox nommé « OptionsBox », on en crée un (sinon aucun bouton Vendre/Stock). */
	void EnsureRuntimeOptionsBox();

	void RebuildOptionButtons();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> TargetObject = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTheHouseRTSUIClickRelay>> ClickRelays;
};

