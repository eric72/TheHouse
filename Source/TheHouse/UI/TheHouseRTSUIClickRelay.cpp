#include "TheHouseRTSUIClickRelay.h"

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
	default:
		break;
	}
}
