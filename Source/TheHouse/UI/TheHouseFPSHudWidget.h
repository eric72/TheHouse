#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseFPSHudWidget.generated.h"

/**
 * HUD minimal en mode FPS (distinct du panneau RTS).
 */
UCLASS()
class THEHOUSE_API UTheHouseFPSHudWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

private:
	void RebuildRootIfNeeded();

	UPROPERTY(Transient)
	TObjectPtr<class UCanvasPanel> RootCanvas = nullptr;
};
