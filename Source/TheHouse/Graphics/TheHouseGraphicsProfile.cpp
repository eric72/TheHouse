#include "TheHouseGraphicsProfile.h"

#include "TheHouseGameUserSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogTheHouseGraphics, Log, All);

namespace
{

void ApplyConsoleVariablesFromIniFile(const FString& IniPath)
{
	if (!FPaths::FileExists(IniPath))
	{
		UE_LOG(LogTheHouseGraphics, Warning, TEXT("INI profil graphique introuvable : %s"), *IniPath);
		return;
	}

	FString Raw;
	if (!FFileHelper::LoadFileToString(Raw, *IniPath))
	{
		UE_LOG(LogTheHouseGraphics, Warning, TEXT("Lecture impossible : %s"), *IniPath);
		return;
	}

	TArray<FString> Lines;
	Raw.ParseIntoArrayLines(Lines, false);

	bool bInConsoleVariables = false;
	int32 Applied = 0;
	int32 Skipped = 0;
	IConsoleManager& ConsoleMgr = IConsoleManager::Get();

	for (FString Line : Lines)
	{
		Line.TrimStartAndEndInline();
		if (Line.IsEmpty() || Line.StartsWith(TEXT(";")))
		{
			continue;
		}
		if (Line.StartsWith(TEXT("[")))
		{
			bInConsoleVariables = Line.Equals(TEXT("[ConsoleVariables]"), ESearchCase::IgnoreCase);
			continue;
		}
		if (!bInConsoleVariables)
		{
			continue;
		}

		int32 Eq = INDEX_NONE;
		if (!Line.FindChar(TEXT('='), Eq) || Eq <= 0)
		{
			continue;
		}

		const FString Key = Line.Left(Eq).TrimStartAndEnd();
		const FString Value = Line.Mid(Eq + 1).TrimStartAndEnd();
		if (Key.IsEmpty())
		{
			continue;
		}

		IConsoleVariable* CVar = ConsoleMgr.FindConsoleVariable(*Key, false);
		if (!CVar)
		{
			++Skipped;
			continue;
		}

		CVar->Set(*Value, ECVF_SetByCode);
		++Applied;
	}

	UE_LOG(LogTheHouseGraphics, Log, TEXT("Profil graphique %s : %d CVars appliqués, %d clés ignorées (inconnues)."),
		*FPaths::GetCleanFilename(IniPath), Applied, Skipped);
}

} // namespace

namespace TheHouseGraphicsProfile
{

bool IsHighProfileAllowed()
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return true;
#endif
}

FString GetProfileIniPath(ETheHouseGraphicsProfile Profile)
{
	const TCHAR* Leaf = (Profile == ETheHouseGraphicsProfile::High) ? TEXT("GraphicsProfileHigh.ini")
																	 : TEXT("GraphicsProfileLow.ini");
	return FPaths::Combine(FPaths::ProjectConfigDir(), Leaf);
}

void ApplyIniProfile(ETheHouseGraphicsProfile Profile)
{
	ApplyConsoleVariablesFromIniFile(GetProfileIniPath(Profile));
}

ETheHouseGraphicsProfile ResolveStartupProfile()
{
	FString CmdValue;
	if (FParse::Value(FCommandLine::Get(), TEXT("-TheHouseGraphicsProfile="), CmdValue))
	{
		if (CmdValue.Equals(TEXT("High"), ESearchCase::IgnoreCase) && IsHighProfileAllowed())
		{
			return ETheHouseGraphicsProfile::High;
		}
		return ETheHouseGraphicsProfile::Low;
	}

	if (UGameUserSettings* BaseGus = UGameUserSettings::GetGameUserSettings())
	{
		BaseGus->LoadSettings(false);
	}
	if (UTheHouseGameUserSettings* TH = UTheHouseGameUserSettings::GetTheHouseGameUserSettings())
	{
		TH->MigrateLegacyGraphicsIniIfPresent();
		ETheHouseGraphicsProfile P = TH->GetGraphicsQuality();
		if (P == ETheHouseGraphicsProfile::High && !IsHighProfileAllowed())
		{
			return ETheHouseGraphicsProfile::Low;
		}
		return P;
	}

#if UE_BUILD_SHIPPING
	return ETheHouseGraphicsProfile::Low;
#else
	return ETheHouseGraphicsProfile::High;
#endif
}

} // namespace TheHouseGraphicsProfile
