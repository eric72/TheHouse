#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseRTSContextMenuWidget.generated.h"

class ATheHouseObject;
class ATheHousePlayerController;
class SBorder;
class SVerticalBox;

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
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	void RebuildOptionButtons();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> TargetObject;

	TSharedPtr<SBorder> RootBorder;
	TSharedPtr<SVerticalBox> OptionsSlateBox;
};
