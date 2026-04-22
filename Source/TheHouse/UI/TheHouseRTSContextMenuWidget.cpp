#include "TheHouseRTSContextMenuWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateColor.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#include "TheHouseObject.h"
#include "TheHousePlayerController.h"
#include "TheHouseRTSUITypes.h"

TSharedRef<SWidget> UTheHouseRTSContextMenuWidget::RebuildWidget()
{
	OptionsSlateBox = SNew(SVerticalBox);

	RootBorder =
		SNew(SBorder)
		.Padding(FMargin(8.f))
		.BorderBackgroundColor(FLinearColor(0.05f, 0.05f, 0.06f, 0.92f))
		[
			OptionsSlateBox.ToSharedRef()
		];

	RebuildOptionButtons();

	return RootBorder.ToSharedRef();
}

void UTheHouseRTSContextMenuWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	RootBorder.Reset();
	OptionsSlateBox.Reset();
}

void UTheHouseRTSContextMenuWidget::OpenForTarget(ATheHousePlayerController* InOwnerPC, ATheHouseObject* InTarget, const FVector2D& ScreenPosition)
{
	OwnerPC = InOwnerPC;
	TargetObject = InTarget;

	RebuildOptionButtons();

	SetVisibility(ESlateVisibility::Visible);
	SetRenderOpacity(1.f);
	SetIsEnabled(true);

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

	SetPositionInViewport(Pos, /*bRemoveDPIScale*/ false);
	InvalidateLayoutAndVolatility();
	TSharedRef<SWidget> Slate = TakeWidget();
	Slate->SlatePrepass();
	ForceLayoutPrepass();
}

void UTheHouseRTSContextMenuWidget::RebuildOptionButtons()
{
	if (!OptionsSlateBox.IsValid())
	{
		return;
	}

	OptionsSlateBox->ClearChildren();

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

		const FText LabelText = Def.Label.IsEmpty() ? FText::FromName(Def.OptionId) : Def.Label;
		const FName OptionId = Def.OptionId;
		const TWeakObjectPtr<ATheHousePlayerController> WeakPC = OwnerPC;
		const TWeakObjectPtr<ATheHouseObject> WeakTarget = TargetObject;

		const FButtonStyle& BtnStyle = FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

		// Un seul enfant STextBlock en HitTestInvisible : évite tout SBorder intermédiaire
		// qui pourrait intercepter le hit-test selon la version Slate/UMG.
		OptionsSlateBox->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0.f, 2.f))
		[
			SNew(SButton)
			.ButtonStyle(&BtnStyle)
			.Cursor(EMouseCursor::Hand)
			.ContentPadding(FMargin(10.f, 6.f))
			.OnClicked_Lambda([WeakPC, WeakTarget, OptionId]()
			{
				if (ATheHousePlayerController* PC = WeakPC.Get())
				{
					PC->ExecuteRTSContextMenuOption(OptionId, WeakTarget.Get());
				}
				return FReply::Handled();
			})
			[
				SNew(STextBlock)
				.Text(LabelText)
				.ColorAndOpacity(FSlateColor(FLinearColor::White))
				.Visibility(EVisibility::HitTestInvisible)
			]
		];
	}
}
