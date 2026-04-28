#include "TheHouseRTSMainWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Internationalization/Text.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateTypes.h" // FSlateChildSize, ESlateSizeRule
#include "TheHouseObject.h"
#include "TheHousePlayerController.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouseRTSUIClickRelay.h"
#include "TheHouseRTSUITypes.h"

namespace TheHouseRTSMainWidgetInternal
{
	static constexpr float CatalogThumbSize = 40.f;

	static UTexture2D* FindCatalogThumbnail(
		const TArray<FTheHousePlaceableCatalogEntry>& Catalog,
		TSubclassOf<ATheHouseObject> ObjectClass)
	{
		if (!ObjectClass)
		{
			return nullptr;
		}
		for (const FTheHousePlaceableCatalogEntry& Entry : Catalog)
		{
			if (Entry.ObjectClass == ObjectClass && Entry.Thumbnail)
			{
				return Entry.Thumbnail;
			}
		}
		return nullptr;
	}

	static UTexture2D* FindStaffPaletteThumbnail(
		const TArray<FTheHouseNPCStaffPaletteEntry>& Palette,
		TSubclassOf<ATheHouseNPCCharacter> NPCClass)
	{
		if (!NPCClass)
		{
			return nullptr;
		}
		for (const FTheHouseNPCStaffPaletteEntry& E : Palette)
		{
			if (E.NPCClass == NPCClass && E.Thumbnail)
			{
				return E.Thumbnail;
			}
		}
		return nullptr;
	}

	/** WBP : le `ScrollBox` peut être imbriqué (ex. panel latéral) sans BindWidgetOptional — le nom d’instance doit correspondre. */
	static UScrollBox* FindScrollBoxRecursive(UWidget* W, const FName& TargetName)
	{
		if (!W)
		{
			return nullptr;
		}
		if (W->GetFName() == TargetName)
		{
			return Cast<UScrollBox>(W);
		}
		if (UUserWidget* UW = Cast<UUserWidget>(W))
		{
			if (UWidgetTree* InnerTree = UW->WidgetTree.Get())
			{
				if (UWidget* InnerRoot = InnerTree->RootWidget)
				{
					if (UScrollBox* Found = FindScrollBoxRecursive(InnerRoot, TargetName))
					{
						return Found;
					}
				}
			}
		}
		if (UPanelWidget* Panel = Cast<UPanelWidget>(W))
		{
			const int32 Num = Panel->GetChildrenCount();
			for (int32 i = 0; i < Num; ++i)
			{
				if (UWidget* Child = Panel->GetChildAt(i))
				{
					if (UScrollBox* Found = FindScrollBoxRecursive(Child, TargetName))
					{
						return Found;
					}
				}
			}
		}
		return nullptr;
	}
}

void UTheHouseRTSMainWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildRootIfNeeded();
	RefreshAll();
}

bool UTheHouseRTSMainWidget::EscapeCloseOverlays_Implementation()
{
	return false;
}

void UTheHouseRTSMainWidget::RebuildRootIfNeeded()
{
	UWidgetTree* Tree = WidgetTree.Get();
	if (!Tree)
	{
		return;
	}

	// WBP : trois widgets nommés MoneyText, CatalogScroll, StoredScroll (BindWidgetOptional).
	if (MoneyText && CatalogScroll && StoredScroll)
	{
		RootCanvas = Cast<UCanvasPanel>(Tree->RootWidget);
		if (!StaffPaletteScroll && Tree->RootWidget)
		{
			StaffPaletteScroll = TheHouseRTSMainWidgetInternal::FindScrollBoxRecursive(Tree->RootWidget, FName(TEXT("StaffPaletteScroll")));
			if (!StaffPaletteScroll)
			{
				StaffPaletteScroll = TheHouseRTSMainWidgetInternal::FindScrollBoxRecursive(Tree->RootWidget, FName(TEXT("StaffListScroll")));
			}
		}
		return;
	}

	if (Tree->RootWidget)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Error,
			TEXT("[RTS] WBP avec racine designer mais sans BindWidgetOptional complet. "
				"Ajoutez au moins MoneyText, CatalogScroll, StoredScroll (et optionnellement StaffPaletteScroll), "
				"ou définissez RTS Main Widget Class sur la classe C++ UTheHouseRTSMainWidget (sans enfant WBP vide)."));
#endif
		RootCanvas = Cast<UCanvasPanel>(Tree->RootWidget);
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

	auto AddTitle = [Tree, Col](const FText& Title)
	{
		UTextBlock* T = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		T->SetText(Title);
		T->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Col->AddChildToVerticalBox(T);
	};

	// Money row
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(NSLOCTEXT("TheHouse", "RTS_MoneyLabel", "Argent"));
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.85f, 0.9f)));
		Col->AddChildToVerticalBox(Label);

		MoneyText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RTSMoneyText"));
		MoneyText->SetText(FText::AsNumber(0));
		MoneyText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Col->AddChildToVerticalBox(MoneyText);
	}

	AddTitle(NSLOCTEXT("TheHouse", "RTS_CatalogSectionTitle", "Objets (catalogue)"));
	CatalogScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	CatalogScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	CatalogScroll->SetOrientation(EOrientation::Orient_Vertical);
	if (UVerticalBoxSlot* VSlot = Col->AddChildToVerticalBox(CatalogScroll))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	AddTitle(NSLOCTEXT("TheHouse", "RTS_StockSectionTitle", "Stock"));
	StoredScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
	StoredScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	StoredScroll->SetOrientation(EOrientation::Orient_Vertical);
	if (UVerticalBoxSlot* VSlot = Col->AddChildToVerticalBox(StoredScroll))
	{
		VSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	AddTitle(NSLOCTEXT("TheHouse", "RTS_StaffSectionTitle", "Personnel"));
	StaffPaletteScroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("StaffPaletteScroll"));
	StaffPaletteScroll->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	StaffPaletteScroll->SetOrientation(EOrientation::Orient_Vertical);
	if (UVerticalBoxSlot* VSlot = Col->AddChildToVerticalBox(StaffPaletteScroll))
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
	RefreshMoney();
	RebuildCatalogButtons();
	RebuildStoredButtons();
	RebuildStaffPaletteButtons();
	BP_OnAfterRTSMainWidgetRefreshed();
}

void UTheHouseRTSMainWidget::RefreshMoney()
{
	if (!MoneyText)
	{
		return;
	}
	if (!OwnerPC)
	{
		MoneyText->SetText(NSLOCTEXT("TheHouse", "RTS_MoneyUnavailable", "-"));
		return;
	}
	MoneyText->SetText(FText::AsNumber(OwnerPC->GetMoney()));
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
		Btn->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
		Btn->SetClickMethod(EButtonClickMethod::MouseDown);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UTexture2D* Thumb = TheHouseRTSMainWidgetInternal::FindCatalogThumbnail(Entries, E.ObjectClass))
		{
			UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
			Img->SetBrushFromTexture(Thumb, true);
			Img->SetDesiredSizeOverride(FVector2D(TheHouseRTSMainWidgetInternal::CatalogThumbSize, TheHouseRTSMainWidgetInternal::CatalogThumbSize));
			Img->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UHorizontalBoxSlot* ImgSlot = Row->AddChildToHorizontalBox(Img))
			{
				ImgSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
				ImgSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		FText TitleText = E.DisplayName.IsEmpty() ? FText::FromString(E.ObjectClass->GetName()) : E.DisplayName;
		const int32 Price = OwnerPC->GetPurchasePriceForClass(E.ObjectClass);
		if (Price > 0)
		{
			TitleText = FText::Format(
				NSLOCTEXT("TheHouse", "RTSCatalogRowWithPrice", "{0} — {1}"),
				TitleText,
				FText::AsNumber(Price));
		}
		Label->SetText(TitleText);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Label))
		{
			FSlateChildSize TextFill;
			TextFill.SizeRule = ESlateSizeRule::Fill;
			TextFill.Value = 1.f;
			TextSlot->SetSize(TextFill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		Btn->AddChild(Row);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::Catalog;
		Relay->OwnerPC = OwnerPC;
		Relay->CatalogClass = E.ObjectClass;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);
		Btn->SetIsEnabled(OwnerPC->CanAffordCatalogPurchaseForClass(E.ObjectClass));

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

	const TArray<FTheHousePlaceableCatalogEntry>& Catalog = OwnerPC->GetPlaceableCatalog();
	const TArray<FTheHouseStoredStack>& Stacks = OwnerPC->GetStoredPlaceableStacks();
	for (int32 i = 0; i < Stacks.Num(); ++i)
	{
		const FTheHouseStoredStack& Stack = Stacks[i];
		if (!Stack.ObjectClass || Stack.Count <= 0)
		{
			continue;
		}

		FText Title = FText::FromString(Stack.ObjectClass->GetName());
		for (const FTheHousePlaceableCatalogEntry& E : Catalog)
		{
			if (E.ObjectClass == Stack.ObjectClass && !E.DisplayName.IsEmpty())
			{
				Title = E.DisplayName;
				break;
			}
		}
		const FText Line = FText::Format(
			NSLOCTEXT("TheHouse", "RTStockRow", "{0} × {1}"),
			Title,
			FText::AsNumber(Stack.Count));

		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
		Btn->SetClickMethod(EButtonClickMethod::MouseDown);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		if (UTexture2D* Thumb = TheHouseRTSMainWidgetInternal::FindCatalogThumbnail(Catalog, Stack.ObjectClass))
		{
			UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
			Img->SetBrushFromTexture(Thumb, true);
			Img->SetDesiredSizeOverride(FVector2D(TheHouseRTSMainWidgetInternal::CatalogThumbSize, TheHouseRTSMainWidgetInternal::CatalogThumbSize));
			Img->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UHorizontalBoxSlot* ImgSlot = Row->AddChildToHorizontalBox(Img))
			{
				ImgSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
				ImgSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(Line);
		Label->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Label))
		{
			FSlateChildSize TextFill;
			TextFill.SizeRule = ESlateSizeRule::Fill;
			TextFill.Value = 1.f;
			TextSlot->SetSize(TextFill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		Btn->AddChild(Row);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::Stored;
		Relay->OwnerPC = OwnerPC;
		Relay->StoredIndex = i;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);

		V->AddChildToVerticalBox(Btn);
	}
}

void UTheHouseRTSMainWidget::RebuildStaffPaletteButtons()
{
	UWidgetTree* Tree = WidgetTree.Get();
	if (!StaffPaletteScroll && Tree && Tree->RootWidget)
	{
		StaffPaletteScroll = TheHouseRTSMainWidgetInternal::FindScrollBoxRecursive(Tree->RootWidget, FName(TEXT("StaffPaletteScroll")));
		if (!StaffPaletteScroll)
		{
			StaffPaletteScroll = TheHouseRTSMainWidgetInternal::FindScrollBoxRecursive(Tree->RootWidget, FName(TEXT("StaffListScroll")));
		}
	}

	if (!StaffPaletteScroll || !Tree)
	{
		return;
	}

	StaffPaletteScroll->ClearChildren();

	if (!OwnerPC)
	{
		return;
	}

	UVerticalBox* V = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
	StaffPaletteScroll->AddChild(V);

	const TArray<FTheHouseNPCStaffRosterOffer>& Roster = OwnerPC->GetNPCStaffRosterOffers();
	const TArray<FTheHouseNPCStaffPaletteEntry>& Palette = OwnerPC->GetNPCStaffPalette();

	int32 NumButtons = 0;

	auto ApplyStaffPaletteRowStyle = [this](UTextBlock* Label)
	{
		if (!Label)
		{
			return;
		}
		if (StaffPaletteRowFontSize > 0)
		{
			FSlateFontInfo FontInfo = Label->GetFont();
			FontInfo.Size = static_cast<float>(FMath::Clamp(StaffPaletteRowFontSize, 1, 96));
			Label->SetFont(FontInfo);
		}
		Label->SetColorAndOpacity(StaffPaletteRowTextColor);
	};

	auto AddHint = [&](const FText& Msg)
	{
		UTextBlock* Hint = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Hint->SetText(Msg);
		Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.75f, 0.8f)));
		Hint->SetAutoWrapText(true);
		Hint->SetVisibility(ESlateVisibility::HitTestInvisible);
		V->AddChildToVerticalBox(Hint);
	};

	auto BuildRow = [&](
		FText TitleText,
		TSubclassOf<ATheHouseNPCCharacter> NPCClassForThumb,
		int32 Cost,
		int32 RosterIdxOrNeg)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
		Btn->SetClickMethod(EButtonClickMethod::MouseDown);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTexture2D* Thumb = nullptr;
		if (RosterIdxOrNeg >= 0 && Roster.IsValidIndex(RosterIdxOrNeg))
		{
			Thumb = Roster[RosterIdxOrNeg].Thumbnail;
		}
		if (!Thumb)
		{
			Thumb = TheHouseRTSMainWidgetInternal::FindStaffPaletteThumbnail(Palette, NPCClassForThumb);
		}
		if (Thumb)
		{
			UImage* Img = Tree->ConstructWidget<UImage>(UImage::StaticClass());
			Img->SetBrushFromTexture(Thumb, true);
			Img->SetDesiredSizeOverride(FVector2D(TheHouseRTSMainWidgetInternal::CatalogThumbSize, TheHouseRTSMainWidgetInternal::CatalogThumbSize));
			Img->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UHorizontalBoxSlot* ImgSlot = Row->AddChildToHorizontalBox(Img))
			{
				ImgSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
				ImgSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(TitleText);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		ApplyStaffPaletteRowStyle(Label);
		if (UHorizontalBoxSlot* TextSlot = Row->AddChildToHorizontalBox(Label))
		{
			FSlateChildSize TextFill;
			TextFill.SizeRule = ESlateSizeRule::Fill;
			TextFill.Value = 1.f;
			TextSlot->SetSize(TextFill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		Btn->AddChild(Row);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::StaffPalette;
		Relay->OwnerPC = OwnerPC;
		Relay->StaffRosterOfferIndex = RosterIdxOrNeg;
		Relay->StaffNPCClass = NPCClassForThumb;
		Relay->StaffNPCPaletteHireCost = Cost;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);
		Btn->SetIsEnabled(Cost == 0 || OwnerPC->GetMoney() >= Cost);

		V->AddChildToVerticalBox(Btn);
		++NumButtons;
	};

	auto BuildCategoryRow = [&](const FText& TitleText, const FName CategoryId)
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
		Btn->SetClickMethod(EButtonClickMethod::MouseDown);

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(TitleText);
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		ApplyStaffPaletteRowStyle(Label);
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::StaffRecruitmentPickCategory;
		Relay->OwnerPC = OwnerPC;
		Relay->StaffRecruitmentCategoryId = CategoryId;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);

		V->AddChildToVerticalBox(Btn);
		++NumButtons;
	};

	auto BuildBackRow = [&]()
	{
		UButton* Btn = Tree->ConstructWidget<UButton>(UButton::StaticClass());
		Btn->SetStyle(FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button"));
		Btn->SetClickMethod(EButtonClickMethod::MouseDown);

		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		Label->SetText(NSLOCTEXT("TheHouse", "RTSStaffRecruitmentBack", "← Métiers"));
		Label->SetVisibility(ESlateVisibility::HitTestInvisible);
		ApplyStaffPaletteRowStyle(Label);
		Btn->AddChild(Label);

		UTheHouseRTSUIClickRelay* Relay = NewObject<UTheHouseRTSUIClickRelay>(this);
		Relay->Kind = ETheHouseRTSUIClickKind::StaffRecruitmentBackToCategories;
		Relay->OwnerPC = OwnerPC;
		ClickRelays.Add(Relay);
		Btn->OnClicked.AddDynamic(Relay, &UTheHouseRTSUIClickRelay::RelayClicked);

		V->AddChildToVerticalBox(Btn);
		++NumButtons;
	};

	if (Roster.Num() > 0)
	{
		const FName Browse = OwnerPC->GetStaffUiRosterBrowseCategoryId();
		if (Browse.IsNone())
		{
			TMap<FName, FText> CategoryLabels;
			for (const FTheHouseNPCStaffRosterOffer& O : Roster)
			{
				if (!O.CharacterClass)
				{
					continue;
				}
				if (!CategoryLabels.Contains(O.StaffCategoryId))
				{
					const FText L = O.StaffCategoryLabel.IsEmpty() ? FText::FromName(O.StaffCategoryId) : O.StaffCategoryLabel;
					CategoryLabels.Add(O.StaffCategoryId, L);
				}
			}

			TArray<FName> CategoryKeys;
			CategoryLabels.GetKeys(CategoryKeys);
			CategoryKeys.Sort([](const FName& A, const FName& B) {
				return A.ToString().Compare(B.ToString()) < 0;
			});

			for (const FName& Key : CategoryKeys)
			{
				BuildCategoryRow(CategoryLabels[Key], Key);
			}
		}
		else
		{
			BuildBackRow();

			for (int32 i = 0; i < Roster.Num(); ++i)
			{
				const FTheHouseNPCStaffRosterOffer& O = Roster[i];
				if (!O.CharacterClass || O.StaffCategoryId != Browse)
				{
					continue;
				}

				const int32 Cost = FMath::Max(0, O.HireCost);
				if (Cost > 0 && OwnerPC->GetMoney() < Cost)
				{
					continue;
				}

				FText TitleText = O.DisplayName.IsEmpty() ? FText::FromString(O.CharacterClass->GetName()) : O.DisplayName;
				if (Cost > 0)
				{
					TitleText = FText::Format(
						NSLOCTEXT("TheHouse", "RTSStaffRosterRow", "{0} · ★{1} · {2}/mo — {3}"),
						TitleText,
						O.StarRating,
						FText::AsNumber(FMath::RoundToInt(O.MonthlySalary)),
						FText::AsNumber(Cost));
				}
				else
				{
					TitleText = FText::Format(
						NSLOCTEXT("TheHouse", "RTSStaffRosterRowFree", "{0} · ★{1} · {2}/mo"),
						TitleText,
						O.StarRating,
						FText::AsNumber(FMath::RoundToInt(O.MonthlySalary)));
				}

				BuildRow(TitleText, O.CharacterClass, Cost, i);
			}
		}
	}
	else
	{
		for (int32 i = 0; i < Palette.Num(); ++i)
		{
			const FTheHouseNPCStaffPaletteEntry& E = Palette[i];
			if (!E.NPCClass)
			{
				continue;
			}

			const int32 Cost = FMath::Max(0, E.HireCost);
			if (Cost > 0 && OwnerPC->GetMoney() < Cost)
			{
				continue;
			}

			FText TitleText = E.DisplayName.IsEmpty() ? FText::FromString(E.NPCClass->GetName()) : E.DisplayName;
			if (Cost > 0)
			{
				TitleText = FText::Format(
					NSLOCTEXT("TheHouse", "RTSStaffRowWithPrice", "{0} — {1}"),
					TitleText,
					FText::AsNumber(Cost));
			}

			BuildRow(TitleText, E.NPCClass, Cost, INDEX_NONE);
		}
	}

	if (NumButtons == 0)
	{
		if (Roster.Num() > 0 || Palette.Num() > 0)
		{
			AddHint(NSLOCTEXT(
				"TheHouse",
				"RTSStaffEmptyFiltered",
				"Aucune ligne affichable (budget insuffisant ou filtre vide). Rafraîchissez le recrutement ou augmentez l’argent."));
		}
		else
		{
			AddHint(NSLOCTEXT(
				"TheHouse",
				"RTSStaffEmptyConfig",
				"Aucun personnel : renseignez « NPC Staff Recruitment Pool » ou « NPC Staff Palette » sur le PlayerController, "
				"ou placez un UScrollBox nommé StaffPaletteScroll dans ce panneau."));
		}
	}
}
