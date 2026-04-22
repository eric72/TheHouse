#include "TheHouseRTSContextMenuUMGWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Styling/AppStyle.h"
#include "TheHouseObject.h"
#include "TheHousePlayerController.h"
#include "TheHouseRTSUIClickRelay.h"
#include "TheHouseRTSUITypes.h"

void UTheHouseRTSContextMenuUMGWidget::EnsureRuntimeOptionsBox()
{
	if (OptionsBox)
	{
		return;
	}

	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	OptionsBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsBox_Auto"));
	if (!OptionsBox)
	{
		return;
	}

	OptionsBox->SetVisibility(ESlateVisibility::Visible);

	if (MenuBorder)
	{
		MenuBorder->SetContent(OptionsBox);
		return;
	}

	if (UBorder* RootAsBorder = Cast<UBorder>(Tree->RootWidget))
	{
		MenuBorder = RootAsBorder;
		MenuBorder->SetContent(OptionsBox);
		return;
	}

	if (!Tree->RootWidget)
	{
		MenuBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBorder_Auto"));
		MenuBorder->SetContent(OptionsBox);
		Tree->RootWidget = MenuBorder;
		return;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Tree->RootWidget))
	{
		Panel->AddChild(OptionsBox);
	}
}

void UTheHouseRTSContextMenuUMGWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// The menu is created dynamically and should reliably receive mouse input.
	// Making it focusable helps in Game+UI input mode where focus can stick to the viewport.
	SetIsFocusable(true);
	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();
}

void UTheHouseRTSContextMenuUMGWidget::OpenForTarget(ATheHousePlayerController* InOwnerPC, ATheHouseObject* InTarget, const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetObject = InTarget;

	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();

	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);
	SetRenderOpacity(1.f);
	SetAlignmentInViewport(FVector2D(0.f, 0.f));
	// Coordonnées dans l’espace local du widget viewport (ex. GetMousePositionOnViewport).
	FVector2D Pos = ScreenPosition;
	if (UWorld* W = GetWorld())
	{
		const FVector2D VS = UWidgetLayoutLibrary::GetViewportSize(W);
		if (VS.X > 0.f && VS.Y > 0.f)
		{
			Pos.X = FMath::Clamp(Pos.X, 0.f, VS.X - 10.f);
			Pos.Y = FMath::Clamp(Pos.Y, 0.f, VS.Y - 10.f);
		}
	}
	SetPositionInViewport(Pos, /*bRemoveDPIScale*/ false);
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTheHouseRTSContextMenuUMGWidget::RebuildOptionButtons()
{
	ClickRelays.Empty();

	EnsureRuntimeOptionsBox();
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

	if (!OwnerPC || !TargetObject)
	{
		return;
	}

	const TArray<FTheHouseRTSContextMenuOptionDef>& Defs = OwnerPC->GetRTSContextMenuOptionDefs();
	int32 BuiltCount = 0;
	for (const FTheHouseRTSContextMenuOptionDef& Def : Defs)
	{
		if (Def.OptionId.IsNone())
		{
			continue;
		}

		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
		Btn->SetClickMethod(EButtonClickMethod::MouseDown);
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Def.Label.IsEmpty() ? FText::FromName(Def.OptionId) : Def.Label);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		// Le texte ne doit pas intercepter le clic : le relais est sur le UButton.
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::ContextOption;
		Relay->OwnerPC = OwnerPC;
		Relay->ContextTarget = TargetObject;
		Relay->ContextOptionId = Def.OptionId;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);
		Btn->SetVisibility(ESlateVisibility::Visible);

		OptionsBox->AddChildToVerticalBox(Btn);
		++BuiltCount;
	}

#if !UE_BUILD_SHIPPING
	if (BuiltCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("[RTSContext] Aucun bouton de menu (OptionsBox ou RTSContextMenuOptionDefs). Vérifie le PlayerController / le Widget du menu."));
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[RTSContext] Menu options built=%d"), BuiltCount);
	}
#endif
}

