#include "TheHouseSettingsMenuWidget.h"

#include "TheHouseGameUserSettings.h"
#include "TheHousePlayerController.h"

void UTheHouseSettingsMenuWidget::BindToPlayerController(ATheHousePlayerController* PC)
{
	OwnerPC = PC;
}

UTheHouseGameUserSettings* UTheHouseSettingsMenuWidget::GetTheHouseGameUserSettings() const
{
	return UTheHouseGameUserSettings::GetTheHouseGameUserSettings();
}

void UTheHouseSettingsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BP_OnSettingsMenuConstructed();
}
