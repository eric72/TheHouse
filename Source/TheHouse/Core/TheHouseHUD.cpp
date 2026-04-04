#include "TheHouseHUD.h"
#include "Engine/Canvas.h"

ATheHouseHUD::ATheHouseHUD()
{
	bIsSelecting = false;
	SelectionBoxColor = FLinearColor(0.0f, 1.0f, 0.0f, 0.2f); // Semi-transparent Green
}

void ATheHouseHUD::DrawHUD()
{
	Super::DrawHUD();

	if (bIsSelecting)
	{
		// UE_LOG(LogTemp, Warning, TEXT("Drawing Selection HUD.")); 
		FVector2D TopLeft;
		FVector2D Dimensions;

		if (GetSelectionRectangle(TopLeft, Dimensions))
		{
			// Draw the filled rectangle
			DrawRect(SelectionBoxColor, TopLeft.X, TopLeft.Y, Dimensions.X, Dimensions.Y);

			// Optionally draw a border
			DrawLine(TopLeft.X, TopLeft.Y, TopLeft.X + Dimensions.X, TopLeft.Y, FLinearColor(0.f, 1.f, 0.f, 1.f)); // Top
			DrawLine(TopLeft.X, TopLeft.Y + Dimensions.Y, TopLeft.X + Dimensions.X, TopLeft.Y + Dimensions.Y, FLinearColor(0.f, 1.f, 0.f, 1.f)); // Bottom
			DrawLine(TopLeft.X, TopLeft.Y, TopLeft.X, TopLeft.Y + Dimensions.Y, FLinearColor(0.f, 1.f, 0.f, 1.f)); // Left
			DrawLine(TopLeft.X + Dimensions.X, TopLeft.Y, TopLeft.X + Dimensions.X, TopLeft.Y + Dimensions.Y, FLinearColor(0.f, 1.f, 0.f, 1.f)); // Right
		}
	}
}

void ATheHouseHUD::StartSelection(const FVector2D& StartPos)
{
	InitialClickLocation = StartPos;
	CurrentMouseLocation = StartPos;
	bIsSelecting = true;
}

void ATheHouseHUD::UpdateSelection(const FVector2D& CurrentPos)
{
	CurrentMouseLocation = CurrentPos;
}

void ATheHouseHUD::StopSelection()
{
	bIsSelecting = false;
}

bool ATheHouseHUD::GetSelectionRectangle(FVector2D& OutTopLeft, FVector2D& OutDimensions) const
{
	if (!bIsSelecting)
	{
		return false;
	}

	float MinX = FMath::Min(InitialClickLocation.X, CurrentMouseLocation.X);
	float MinY = FMath::Min(InitialClickLocation.Y, CurrentMouseLocation.Y);
	float MaxX = FMath::Max(InitialClickLocation.X, CurrentMouseLocation.X);
	float MaxY = FMath::Max(InitialClickLocation.Y, CurrentMouseLocation.Y);

	OutTopLeft = FVector2D(MinX, MinY);
	OutDimensions = FVector2D(MaxX - MinX, MaxY - MinY);

	return true;
}
