#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseRTSContextMenuWidget.generated.h"

class ATheHouseObject;
class ATheHousePlayerController;
class UTheHouseRTSUIClickRelay;

/**
 * Menu contextuel RTS : options construites depuis le PlayerController.
 */
UCLASS()
class THEHOUSE_API UTheHouseRTSContextMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void OpenForTarget(ATheHousePlayerController* InOwnerPC, ATheHouseObject* InTarget, const FVector2D& ScreenPosition);

protected:
	virtual void NativeConstruct() override;

private:
	void RebuildRootIfNeeded();
	void RebuildOptionButtons();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> TargetObject;

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> MenuBorder = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<class UVerticalBox> OptionsBox = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTheHouseRTSUIClickRelay>> ClickRelays;
};
