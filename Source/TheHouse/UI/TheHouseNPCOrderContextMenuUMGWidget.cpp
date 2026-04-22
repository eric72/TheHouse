#include "TheHouseNPCOrderContextMenuUMGWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Styling/AppStyle.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouseObject.h"
#include "TheHousePlayerController.h"
#include "TheHouseRTSUIClickRelay.h"
#include "TheHouseRTSUITypes.h"

namespace TheHouseNPCOrderMenuInternal
{
	static void PositionMenuInViewport(UUserWidget* Self, const FVector2D& ScreenPosition)
	{
		if (!Self)
		{
			return;
		}
		Self->SetVisibility(ESlateVisibility::Visible);
		Self->SetIsEnabled(true);
		Self->SetRenderOpacity(1.f);
		Self->SetAlignmentInViewport(FVector2D(0.f, 0.f));

		FVector2D Pos = ScreenPosition;
		if (UWorld* W = Self->GetWorld())
		{
			const FVector2D VS = UWidgetLayoutLibrary::GetViewportSize(W);
			if (VS.X > 0.f && VS.Y > 0.f)
			{
				Pos.X = FMath::Clamp(Pos.X, 0.f, VS.X - 10.f);
				Pos.Y = FMath::Clamp(Pos.Y, 0.f, VS.Y - 10.f);
			}
		}
		// ScreenPosition provient de GetMousePositionOnViewport → position en espace viewport.
		// On retire le DPI scale pour éviter un menu « hors écran / invisible » selon la config PIE.
		Self->SetPositionInViewport(Pos, /*bRemoveDPIScale*/ true);
		Self->InvalidateLayoutAndVolatility();
		Self->ForceLayoutPrepass();
	}
}

void UTheHouseNPCOrderContextMenuUMGWidget::EnsureRuntimeOptionsBox()
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

	OptionsBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsBox_NPCOrder_Auto"));
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
		MenuBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBorder_NPCOrder_Auto"));
		// Fallback visuel : sans WBP, le menu serait juste une liste de boutons sans fond (souvent difficile à voir).
		MenuBorder->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.78f));
		MenuBorder->SetPadding(FMargin(10.f, 8.f));
		MenuBorder->SetContent(OptionsBox);
		Tree->RootWidget = MenuBorder;
		return;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Tree->RootWidget))
	{
		Panel->AddChild(OptionsBox);
	}
}

void UTheHouseNPCOrderContextMenuUMGWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();
}

void UTheHouseNPCOrderContextMenuUMGWidget::OpenForOrderOnObject(
	ATheHousePlayerController* InOwnerPC,
	ATheHouseObject* InTargetObject,
	const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetKind = ETheHouseNPCOrderMenuTargetKind::Object;
	TargetObject = InTargetObject;
	TargetNPC = nullptr;

	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();
	TheHouseNPCOrderMenuInternal::PositionMenuInViewport(this, ScreenPosition);
}

void UTheHouseNPCOrderContextMenuUMGWidget::OpenForOrderOnNPC(
	ATheHousePlayerController* InOwnerPC,
	ATheHouseNPCCharacter* InTargetNPC,
	const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetKind = ETheHouseNPCOrderMenuTargetKind::NPC;
	TargetObject = nullptr;
	TargetNPC = InTargetNPC;
	TargetGroundLocation = FVector::ZeroVector;

	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();
	TheHouseNPCOrderMenuInternal::PositionMenuInViewport(this, ScreenPosition);
}

void UTheHouseNPCOrderContextMenuUMGWidget::OpenForOrderOnGround(
	ATheHousePlayerController* InOwnerPC,
	const FVector& InTargetLocation,
	const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetKind = ETheHouseNPCOrderMenuTargetKind::Ground;
	TargetObject = nullptr;
	TargetNPC = nullptr;
	TargetGroundLocation = InTargetLocation;

	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();
	TheHouseNPCOrderMenuInternal::PositionMenuInViewport(this, ScreenPosition);
}

void UTheHouseNPCOrderContextMenuUMGWidget::RebuildOptionButtons()
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

	if (!OwnerPC)
	{
		return;
	}

	const TArray<FTheHouseRTSContextMenuOptionDef>* DefsPtr = nullptr;
	switch (TargetKind)
	{
	case ETheHouseNPCOrderMenuTargetKind::Object:
		if (!TargetObject)
		{
			return;
		}
		DefsPtr = &OwnerPC->GetNPCOrderOnObjectRuntimeOptionDefs();
		break;
	case ETheHouseNPCOrderMenuTargetKind::NPC:
		if (!TargetNPC)
		{
			return;
		}
		DefsPtr = &OwnerPC->GetNPCOrderOnNPCRuntimeOptionDefs();
		break;
	case ETheHouseNPCOrderMenuTargetKind::Ground:
		DefsPtr = &OwnerPC->GetNPCOrderOnGroundRuntimeOptionDefs();
		break;
	default:
		return;
	}

	const TArray<FTheHouseRTSContextMenuOptionDef>& Defs = *DefsPtr;
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
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->OwnerPC = OwnerPC;
		Relay->ContextOptionId = Def.OptionId;
		switch (TargetKind)
		{
		case ETheHouseNPCOrderMenuTargetKind::Object:
			Relay->Kind = ETheHouseRTSUIClickKind::ContextNPCOrderOnObject;
			Relay->ContextTarget = TargetObject;
			break;
		case ETheHouseNPCOrderMenuTargetKind::NPC:
			Relay->Kind = ETheHouseRTSUIClickKind::ContextNPCOrderOnNPC;
			Relay->ContextNPCTarget = TargetNPC;
			break;
		case ETheHouseNPCOrderMenuTargetKind::Ground:
			Relay->Kind = ETheHouseRTSUIClickKind::ContextNPCOrderOnGround;
			break;
		default:
			break;
		}
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);
		Btn->SetVisibility(ESlateVisibility::Visible);

		OptionsBox->AddChildToVerticalBox(Btn);
		++BuiltCount;
	}

#if !UE_BUILD_SHIPPING
	if (BuiltCount == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[RTS|NPCOrder] Aucune ligne (GatherNPCOrderActionsOn* sur le PlayerController / BP). Kind=%d"),
			static_cast<int32>(TargetKind));
	}
#endif
}
