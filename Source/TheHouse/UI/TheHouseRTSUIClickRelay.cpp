#include "TheHouseRTSUIClickRelay.h"

#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouseObject.h"
#include "TheHousePlayerController.h"

void UTheHouseRTSUIClickRelay::RelayClicked()
{
	if (!IsValid(OwnerPC))
	{
		return;
	}

	switch (Kind)
	{
	case ETheHouseRTSUIClickKind::Catalog:
		OwnerPC->StartPlacementPreviewForClass(CatalogClass);
		break;
	case ETheHouseRTSUIClickKind::Stored:
		OwnerPC->ConsumeStoredPlaceableAndBeginPreview(StoredIndex);
		break;
	case ETheHouseRTSUIClickKind::ContextOption:
		if (IsValid(ContextTarget))
		{
			OwnerPC->ExecuteRTSContextMenuOption(ContextOptionId, ContextTarget);
		}
		break;
	case ETheHouseRTSUIClickKind::ContextNPCOption:
		if (IsValid(ContextNPCTarget))
		{
			OwnerPC->ExecuteNPCRTSContextMenuOption(ContextOptionId, ContextNPCTarget);
		}
		break;
	case ETheHouseRTSUIClickKind::ContextNPCOrderOnObject:
		if (IsValid(ContextTarget))
		{
			OwnerPC->NotifyRtsOrderMenuOptionClickedFromUi();
			OwnerPC->ExecuteNPCOrderOnObjectAction(ContextOptionId, ContextTarget);
		}
		break;
	case ETheHouseRTSUIClickKind::ContextNPCOrderOnNPC:
		if (IsValid(ContextNPCTarget))
		{
			OwnerPC->NotifyRtsOrderMenuOptionClickedFromUi();
			OwnerPC->ExecuteNPCOrderOnNPCAction(ContextOptionId, ContextNPCTarget);
		}
		break;
	case ETheHouseRTSUIClickKind::ContextNPCOrderOnGround:
		OwnerPC->NotifyRtsOrderMenuOptionClickedFromUi();
		OwnerPC->ExecuteNPCOrderOnGroundAction(ContextOptionId);
		break;
	case ETheHouseRTSUIClickKind::StaffPalette:
		if (StaffRosterOfferIndex >= 0)
		{
			OwnerPC->StartStaffNpcPlacementFromRosterOffer(StaffRosterOfferIndex);
		}
		else
		{
			OwnerPC->StartStaffNpcPlacementFromPalette(StaffNPCClass, StaffNPCPaletteHireCost);
		}
		break;
	default:
		break;
	}
}
