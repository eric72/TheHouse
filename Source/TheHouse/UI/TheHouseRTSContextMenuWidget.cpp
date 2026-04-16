#include "TheHouseRTSContextMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "TheHouseObject.h"
#include "TheHousePlayerController.h"
#include "TheHouseRTSUIClickRelay.h"
#include "TheHouseRTSUITypes.h"

void UTheHouseRTSContextMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildRootIfNeeded();
}

void UTheHouseRTSContextMenuWidget::RebuildRootIfNeeded()
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

	RootCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RTSContextRoot"));
	Tree->RootWidget = RootCanvas;

	MenuBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RTSContextBorder"));
	MenuBorder->SetPadding(FMargin(8.f));
	FSlateBrush Brush;
	Brush.TintColor = FSlateColor(FLinearColor(0.05f, 0.05f, 0.06f, 0.92f));
	MenuBorder->SetBrush(Brush);

	OptionsBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RTSContextOptions"));
	MenuBorder->SetContent(OptionsBox);

	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MenuBorder))
	{
		CanvasSlot->SetAutoSize(true);
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 0.f));
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
	}
}

void UTheHouseRTSContextMenuWidget::OpenForTarget(ATheHousePlayerController* InOwnerPC, ATheHouseObject* InTarget, const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetObject = InTarget;

	RebuildRootIfNeeded();
	RebuildOptionButtons();

	SetVisibility(ESlateVisibility::Visible);

	if (UCanvasPanelSlot* BorderSlot = MenuBorder ? Cast<UCanvasPanelSlot>(MenuBorder->Slot) : nullptr)
	{
		BorderSlot->SetPosition(FVector2D::ZeroVector);
	}

	// Position en pixels viewport (évite un menu « invisible » si le slot canvas ne suit pas le DPI).
	SetPositionInViewport(ScreenPosition, false);
}

void UTheHouseRTSContextMenuWidget::RebuildOptionButtons()
{
	if (!OptionsBox)
	{
		return;
	}

	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	OptionsBox->ClearChildren();
	ClickRelays.Empty();

	if (!OwnerPC || !TargetObject)
	{
		return;
	}

	const TArray<FTheHouseRTSContextMenuOptionDef>& Defs = OwnerPC->GetRTSContextMenuOptionDefs();
	for (const FTheHouseRTSContextMenuOptionDef& Def : Defs)
	{
		if (Def.OptionId.IsNone())
		{
			continue;
		}

		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Def.Label.IsEmpty() ? FText::FromName(Def.OptionId) : Def.Label);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::ContextOption;
		Relay->OwnerPC = OwnerPC;
		Relay->ContextTarget = TargetObject;
		Relay->ContextOptionId = Def.OptionId;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);

		OptionsBox->AddChildToVerticalBox(Btn);
	}
}
