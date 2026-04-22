#include "TheHouseNPCRTSContextMenuUMGWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Styling/AppStyle.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHousePlayerController.h"
#include "TheHouseRTSUIClickRelay.h"
#include "TheHouseRTSUITypes.h"

void UTheHouseNPCRTSContextMenuUMGWidget::EnsureRuntimeOptionsBox()
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

	OptionsBox = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionsBox_NPC_Auto"));
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
		MenuBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuBorder_NPC_Auto"));
		MenuBorder->SetContent(OptionsBox);
		Tree->RootWidget = MenuBorder;
		return;
	}

	if (UPanelWidget* Panel = Cast<UPanelWidget>(Tree->RootWidget))
	{
		Panel->AddChild(OptionsBox);
	}
}

void UTheHouseNPCRTSContextMenuUMGWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();
}

void UTheHouseNPCRTSContextMenuUMGWidget::OpenForNPC(ATheHousePlayerController* InOwnerPC, ATheHouseNPCCharacter* InTargetNPC, const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetNPC = InTargetNPC;

	EnsureRuntimeOptionsBox();
	RebuildOptionButtons();

	SetVisibility(ESlateVisibility::Visible);
	SetIsEnabled(true);
	SetRenderOpacity(1.f);
	SetAlignmentInViewport(FVector2D(0.f, 0.f));

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
	SetPositionInViewport(Pos, false);
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTheHouseNPCRTSContextMenuUMGWidget::RebuildOptionButtons()
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

	if (!OwnerPC || !TargetNPC)
	{
		return;
	}

	const TArray<FTheHouseRTSContextMenuOptionDef>& Defs = OwnerPC->GetNPCRTSContextMenuOptionDefs();
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
		Relay->Kind = ETheHouseRTSUIClickKind::ContextNPCOption;
		Relay->OwnerPC = OwnerPC;
		Relay->ContextNPCTarget = TargetNPC;
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
		UE_LOG(LogTemp, Error, TEXT("[RTSNPCContext] Aucun bouton PNJ (OptionsBox ou NPCRTSContextMenuOptionDefs). Vérifiez le PlayerController et le widget menu."));
	}
#endif
}
