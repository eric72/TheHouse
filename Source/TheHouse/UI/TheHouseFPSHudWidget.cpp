#include "TheHouseFPSHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"

void UTheHouseFPSHudWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildRootIfNeeded();
}

void UTheHouseFPSHudWidget::RebuildRootIfNeeded()
{
	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	if (Tree->RootWidget)
	{
		RootCanvas = Cast<UCanvasPanel>(Tree->RootWidget.Get());
		return;
	}

	RootCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("FPSHudRoot"));
	Tree->RootWidget = RootCanvas;

	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FPSHudPanel"));
	Panel->SetPadding(FMargin(12.f));
	FSlateBrush Brush;
	Brush.TintColor = FSlateColor(FLinearColor(0.f, 0.f, 0.f, 0.35f));
	Panel->SetBrush(Brush);

	UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	T->SetText(FText::FromString(TEXT("Mode FPS — F : retour RTS")));
	T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Panel->SetContent(T);

	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 1.f, 1.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(16.f, -72.f, 16.f, 16.f));
		CanvasSlot->SetAlignment(FVector2D(0.f, 1.f));
	}
}
