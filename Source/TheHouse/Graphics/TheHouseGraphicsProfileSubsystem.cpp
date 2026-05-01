#include "TheHouseGraphicsProfileSubsystem.h"

#include "TheHouseGameUserSettings.h"
#include "TheHouseGraphicsProfile.h"

ETheHouseGraphicsProfile UTheHouseGraphicsProfileSubsystem::GetActiveProfile() const
{
	if (const UTheHouseGameUserSettings* GS = UTheHouseGameUserSettings::GetTheHouseGameUserSettings())
	{
		return GS->GetGraphicsQuality();
	}
	return ActiveProfile;
}

void UTheHouseGraphicsProfileSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (const UTheHouseGameUserSettings* GS = UTheHouseGameUserSettings::GetTheHouseGameUserSettings())
	{
		ActiveProfile = GS->GetGraphicsQuality();
	}
	else
	{
		ActiveProfile = TheHouseGraphicsProfile::ResolveStartupProfile();
	}
	TheHouseGraphicsProfile::ApplyIniProfile(ActiveProfile);
}

bool UTheHouseGraphicsProfileSubsystem::IsHighProfileAllowed() const
{
	return TheHouseGraphicsProfile::IsHighProfileAllowed();
}

void UTheHouseGraphicsProfileSubsystem::SetCachedGraphicsProfile(ETheHouseGraphicsProfile Profile)
{
	ActiveProfile = Profile;
}

void UTheHouseGraphicsProfileSubsystem::SetProfile(ETheHouseGraphicsProfile Profile)
{
	if (Profile == ETheHouseGraphicsProfile::High && !IsHighProfileAllowed())
	{
		Profile = ETheHouseGraphicsProfile::Low;
	}

	ActiveProfile = Profile;
	TheHouseGraphicsProfile::ApplyIniProfile(ActiveProfile);

	if (UTheHouseGameUserSettings* GS = UTheHouseGameUserSettings::GetTheHouseGameUserSettings())
	{
		GS->SetGraphicsQuality(ActiveProfile);
		GS->SaveSettings();
	}
}

void UTheHouseGraphicsProfileSubsystem::ToggleProfile()
{
	const ETheHouseGraphicsProfile Cur =
		UTheHouseGameUserSettings::GetTheHouseGameUserSettings()
			? UTheHouseGameUserSettings::GetTheHouseGameUserSettings()->GetGraphicsQuality()
			: ActiveProfile;

	if (Cur == ETheHouseGraphicsProfile::Low)
	{
		SetProfile(IsHighProfileAllowed() ? ETheHouseGraphicsProfile::High : ETheHouseGraphicsProfile::Low);
	}
	else
	{
		SetProfile(ETheHouseGraphicsProfile::Low);
	}
}
