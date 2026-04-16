#include "TheHouseRTSMainWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "TheHouseObject.h"
#include "TheHousePlayerController.h"
#include "TheHouseRTSUIClickRelay.h"
#include "TheHouseRTSUITypes.h"

void UTheHouseRTSMainWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildRootIfNeeded();
	RefreshAll();
}

void UTheHouseRTSMainWidget::RebuildRootIfNeeded()
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

	RootCanvas = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RTSMainRoot"));
	Tree->RootWidget = RootCanvas;

	UBorder* Panel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RTSMainPanel"));
	Panel->SetPadding(FMargin(12.f));
	FSlateBrush Brush;
	Brush.TintColor = FSlateColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.55f));
	Panel->SetBrush(Brush);

	UVerticalBox* Col = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RTSMainCol"));

	auto AddTitle = [Tree, Col](const TCHAR* Title)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(FText::FromString(Title));
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Col->AddChildToVerticalBox(T);
	};

	AddTitle(TEXT("Objets (catalogue)"));
	CatalogScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	CatalogScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	CatalogScroll->SetOrientation(EOrientation::Orient_Vertical);
	if (UVerticalBoxSlot* VSlot = Col->AddChildToVerticalBox(CatalogScroll))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	AddTitle(TEXT("Stock"));
	StoredScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StoredScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	StoredScroll->SetOrientation(EOrientation::Orient_Vertical);
	if (UVerticalBoxSlot* VSlot = Col->AddChildToVerticalBox(StoredScroll))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	Panel->SetContent(Col);

	if (UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		CanvasSlot->SetAnchors(FAnchors(0.f, 0.f, 0.f, 1.f));
		CanvasSlot->SetOffsets(FMargin(16.f, 16.f, 16.f, 16.f));
		CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetSize(FVector2D(320.f, 0.f));
	}
}

void UTheHouseRTSMainWidget::BindToPlayerController(ATheHousePlayerController* PC)
{
	OwnerPC = PC;
	RefreshAll();
}

void UTheHouseRTSMainWidget::RefreshAll()
{
	ClickRelays.Empty();
	RebuildRootIfNeeded();
	RebuildCatalogButtons();
	RebuildStoredButtons();
}

void UTheHouseRTSMainWidget::RebuildCatalogButtons()
{
	if (!CatalogScroll)
	{
		return;
	}

	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	CatalogScroll->ClearChildren();

	if (!OwnerPC)
	{
		return;
	}

	UVerticalBox* V = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	CatalogScroll->AddChild(V);

	const TArray<FTheHousePlaceableCatalogEntry>& Entries = OwnerPC->GetPlaceableCatalog();
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		const FTheHousePlaceableCatalogEntry& E = Entries[i];
		if (!E.ObjectClass)
		{
			continue;
		}

		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(E.DisplayName.IsEmpty() ? FText::FromString(E.ObjectClass->GetName()) : E.DisplayName);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::Catalog;
		Relay->OwnerPC = OwnerPC;
		Relay->CatalogClass = E.ObjectClass;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);

		V->AddChildToVerticalBox(Btn);
	}
}

void UTheHouseRTSMainWidget::RebuildStoredButtons()
{
	if (!StoredScroll)
	{
		return;
	}

	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	StoredScroll->ClearChildren();

	if (!OwnerPC)
	{
		return;
	}

	UVerticalBox* V = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	StoredScroll->AddChild(V);

	const TArray<TSubclassOf<class ATheHouseObject>>& Stored = OwnerPC->GetStoredPlaceables();
	for (int32 i = 0; i < Stored.Num(); ++i)
	{
		TSubclassOf<class ATheHouseObject> Cls = Stored[i];
		if (!Cls)
		{
			continue;
		}

		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(FText::FromString(FString::Printf(TEXT("%s (%d)"), *Cls->GetName(), i)));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::Stored;
		Relay->OwnerPC = OwnerPC;
		Relay->StoredIndex = i;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);

		V->AddChildToVerticalBox(Btn);
	}
}
