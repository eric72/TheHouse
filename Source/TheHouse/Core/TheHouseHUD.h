#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TheHouseHUD.generated.h"

/**
 * HUD class to handle RTS-style selection box drawing.
 */
UCLASS()
class THEHOUSE_API ATheHouseHUD : public AHUD
{
	GENERATED_BODY()

public:
	ATheHouseHUD();

	// Main draw loop
	virtual void DrawHUD() override;

	// Start drawing the selection box
	void StartSelection(const FVector2D& StartPos);

	// Update the current mouse position for the box
	void UpdateSelection(const FVector2D& CurrentPos);

	// Stop drawing
	void StopSelection();

	// Helper to get the selection rectangle
	bool GetSelectionRectangle(FVector2D& OutTopLeft, FVector2D& OutDimensions) const;

protected:
	// Is the player currently dragging a selection box?
	bool bIsSelecting;

	// Starting position of the click
	FVector2D InitialClickLocation;

	// Current position of the mouse
	FVector2D CurrentMouseLocation;

	// Color of the selection box
	UPROPERTY(EditDefaultsOnly, Category = "Selection")
	FLinearColor SelectionBoxColor;
};
