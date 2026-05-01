#include "TheHouseGameUserSettings.h"

#include "TheHouseGraphicsProfile.h"
#include "TheHouseGraphicsProfileSubsystem.h"
#include "TheHouseUserSettingsAudit.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/InputSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogTheHouseUserSettings, Log, All);

UTheHouseGameUserSettings* UTheHouseGameUserSettings::GetTheHouseGameUserSettings()
{
	return Cast<UTheHouseGameUserSettings>(UGameUserSettings::GetGameUserSettings());
}

void UTheHouseGameUserSettings::SetGraphicsQuality(ETheHouseGraphicsProfile InQuality)
{
#if UE_BUILD_SHIPPING
	if (InQuality == ETheHouseGraphicsProfile::High)
	{
		InQuality = ETheHouseGraphicsProfile::Low;
	}
#endif
	GraphicsQuality = InQuality;
}

void UTheHouseGameUserSettings::SetMasterVolumeNormalized(float V)
{
	MasterVolumeNormalized = FMath::Clamp(V, 0.f, 1.f);
}

void UTheHouseGameUserSettings::SetMusicVolumeNormalized(float V)
{
	MusicVolumeNormalized = FMath::Clamp(V, 0.f, 1.f);
}

void UTheHouseGameUserSettings::SetSfxVolumeNormalized(float V)
{
	SfxVolumeNormalized = FMath::Clamp(V, 0.f, 1.f);
}

void UTheHouseGameUserSettings::TheHouse_SaveToDisk()
{
	SaveSettings();
}

void UTheHouseGameUserSettings::TheHouse_SaveInputMappings()
{
	if (UInputSettings* IS = UInputSettings::GetInputSettings())
	{
		IS->SaveConfig();
	}
	TheHouseUserSettingsAudit::AppendInputRemapNote(TEXT("InputSettings.SaveConfig"));
}

void UTheHouseGameUserSettings::TheHouse_ApplyToRuntime(UWorld* World)
{
	BP_ApplyTheHouseAudioVolumes(MasterVolumeNormalized, MusicVolumeNormalized, SfxVolumeNormalized);

	TheHouseGraphicsProfile::ApplyIniProfile(GraphicsQuality);

	if (World && World->GetGameInstance())
	{
		if (UTheHouseGraphicsProfileSubsystem* Sub =
				World->GetGameInstance()->GetSubsystem<UTheHouseGraphicsProfileSubsystem>())
		{
			Sub->SetCachedGraphicsProfile(GraphicsQuality);
		}
	}

	SaveSettings();
}

void UTheHouseGameUserSettings::MigrateLegacyGraphicsIniIfPresent()
{
	static bool bDidMigrate = false;
	if (bDidMigrate)
	{
		return;
	}
	bDidMigrate = true;

	const FString LegacyPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TheHouseGraphicsUser.ini"));
	if (!FPaths::FileExists(LegacyPath))
	{
		return;
	}

	FConfigFile Legacy;
	Legacy.Read(LegacyPath);
	FString ProfileStr;
	if (Legacy.GetString(TEXT("TheHouse.Graphics"), TEXT("Profile"), ProfileStr))
	{
		if (ProfileStr.Equals(TEXT("High"), ESearchCase::IgnoreCase))
		{
			SetGraphicsQuality(ETheHouseGraphicsProfile::High);
		}
		else
		{
			SetGraphicsQuality(ETheHouseGraphicsProfile::Low);
		}
		UE_LOG(LogTheHouseUserSettings, Log, TEXT("Migration : profil graphique importé depuis %s"), *LegacyPath);
		if (UGameUserSettings* Gus = UGameUserSettings::GetGameUserSettings())
		{
			Gus->SaveSettings();
		}
	}
	IFileManager::Get().Delete(*LegacyPath, false, true);
}

void UTheHouseGameUserSettings::ApplySettings(bool bCheckForResolutionOverride)
{
	Super::ApplySettings(bCheckForResolutionOverride);
	TheHouseGraphicsProfile::ApplyIniProfile(GraphicsQuality);
}

void UTheHouseGameUserSettings::SaveSettings()
{
	Super::SaveSettings();
	TheHouseUserSettingsAudit::AppendSnapshotFromSettings(this);
}

void UTheHouseGameUserSettings::BP_ApplyTheHouseAudioVolumes_Implementation(float Master01, float Music01, float Sfx01)
{
	(void)Master01;
	(void)Music01;
	(void)Sfx01;
}
