#include "TheHousePlacedObjectSettingsWidget.h"

#include "TheHouseObject.h"

void UTheHousePlacedObjectSettingsWidget::SetTargetPlacedObject(ATheHouseObject* Obj)
{
	TargetObject = Obj;
	BP_OnTargetPlacedObjectChanged();
}
