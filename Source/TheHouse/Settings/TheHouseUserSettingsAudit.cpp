#include "TheHouseUserSettingsAudit.h"

#include "TheHouseGameUserSettings.h"
#include "TheHouseGraphicsProfileTypes.h"
#include "HAL/FileManager.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{

FString GetAuditLogPath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TheHouse"), TEXT("settings_history.log"));
}

void EnsureTheHouseSavedDir()
{
	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TheHouse"));
	IFileManager::Get().MakeDirectory(*Dir, true);
}

} // namespace

namespace TheHouseUserSettingsAudit
{

static void AppendLine(const FString& Line)
{
	EnsureTheHouseSavedDir();
	const FString Path = GetAuditLogPath();
	FFileHelper::SaveStringToFile(
		Line + LINE_TERMINATOR,
		*Path,
		FFileHelper::EEncodingOptions::ForceUTF8,
		&IFileManager::Get(),
		FILEWRITE_Append);
}

void AppendSnapshotFromSettings(const UTheHouseGameUserSettings* Settings)
{
	if (!Settings)
	{
		return;
	}
	const TCHAR* Gfx = TEXT("?");
	switch (Settings->GetGraphicsQuality())
	{
	case ETheHouseGraphicsProfile::Low:
		Gfx = TEXT("Low");
		break;
	case ETheHouseGraphicsProfile::High:
		Gfx = TEXT("High");
		break;
	default:
		break;
	}
	const FTimespan UtcOffset = FDateTime::UtcNow() - FDateTime::Now();
	const FDateTime UtcNow = FDateTime::Now() + UtcOffset;
	const FString Iso = UtcNow.ToString(TEXT("%Y-%m-%dT%H:%M:%SZ"));
	const FString Payload = FString::Printf(
		TEXT("%s\tsettings\tgraphics=%s\tmaster=%.3f\tmusic=%.3f\tsfx=%.3f"),
		*Iso,
		Gfx,
		Settings->GetMasterVolumeNormalized(),
		Settings->GetMusicVolumeNormalized(),
		Settings->GetSfxVolumeNormalized());
	AppendLine(Payload);
}

void AppendInputRemapNote(const FString& Note)
{
	const FString Safe = Note.Replace(TEXT("\t"), TEXT(" ")).Replace(TEXT("\r"), TEXT(" ")).Replace(TEXT("\n"), TEXT(" ")).Left(200);
	const FTimespan UtcOffset = FDateTime::UtcNow() - FDateTime::Now();
	const FDateTime UtcNow = FDateTime::Now() + UtcOffset;
	const FString Iso = UtcNow.ToString(TEXT("%Y-%m-%dT%H:%M:%SZ"));
	AppendLine(FString::Printf(TEXT("%s\tinput\t%s"), *Iso, *Safe));
}

} // namespace TheHouseUserSettingsAudit
