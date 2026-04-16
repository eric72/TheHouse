#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseRTSMainWidget.generated.h"

class ATheHousePlayerController;
class UTheHouseRTSUIClickRelay;

/**
 * Panneau RTS : catalogue d’objets + liste stockée (classes).
 */
UCLASS()
class THEHOUSE_API UTheHouseRTSMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToPlayerController(ATheHousePlayerController* PC);
	void RefreshAll();

protected:
	virtual void NativeConstruct() override;

private:
	void RebuildRootIfNeeded();
	void RebuildCatalogButtons();
	void RebuildStoredButtons();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UScrollBox> CatalogScroll = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UScrollBox> StoredScroll = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTheHouseRTSUIClickRelay>> ClickRelays;
};
