#include "TheHouseLocalizationSubsystem.h"
#include "TheHouseLanguageSaveGame.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Kismet/GameplayStatics.h"
#include "Framework/Application/SlateApplication.h"

const TCHAR* UTheHouseLocalizationSubsystem::SaveSlotName = TEXT("TheHouseLanguage");

UTheHouseLocalizationSubsystem::UTheHouseLocalizationSubsystem()
{
	SupportedCultureNames = {TEXT("fr"), TEXT("en")};
}

void UTheHouseLocalizationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (SupportedCultureNames.Num() == 0)
	{
		SupportedCultureNames.Add(TEXT("en"));
		SupportedCultureNames.Add(TEXT("fr"));
	}

	LoadPersistedCulture();
}

bool UTheHouseLocalizationSubsystem::ApplyCultureInternal(const FString& CultureName)
{
	if (CultureName.IsEmpty())
	{
		return false;
	}

	const TArray<FString> Prioritized = FInternationalization::Get().GetPrioritizedCultureNames(CultureName);
	if (Prioritized.Num() == 0)
	{
#if !UE_BUILD_SHIPPING
		UE_LOG(LogTemp, Warning, TEXT("[TheHouse|Localization] Culture inconnue ou sans correspondance : \"%s\""), *CultureName);
#endif
		return false;
	}

	const FString& Resolved = Prioritized[0];
	FInternationalization::Get().SetCurrentCulture(Resolved);

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().InvalidateAllWidgets(false);
	}

	OnCultureChanged.Broadcast();
	return true;
}

bool UTheHouseLocalizationSubsystem::SetLanguage(const FString& CultureName)
{
	const FString Trimmed = CultureName.TrimStartAndEnd();
	if (!ApplyCultureInternal(Trimmed))
	{
		return false;
	}

	PersistCulture(Trimmed);
	return true;
}

FString UTheHouseLocalizationSubsystem::GetCurrentLanguage() const
{
	return FInternationalization::Get().GetCurrentCulture()->GetName();
}

void UTheHouseLocalizationSubsystem::PersistCulture(const FString& CultureName)
{
	UTheHouseLanguageSaveGame* Save =
		Cast<UTheHouseLanguageSaveGame>(UGameplayStatics::CreateSaveGameObject(UTheHouseLanguageSaveGame::StaticClass()));
	if (!Save)
	{
		return;
	}

	Save->PreferredCultureName = CultureName;
	UGameplayStatics::SaveGameToSlot(Save, SaveSlotName, SaveUserIndex);
}

void UTheHouseLocalizationSubsystem::LoadPersistedCulture()
{
	if (!UGameplayStatics::DoesSaveGameExist(SaveSlotName, SaveUserIndex))
	{
		return;
	}

	UTheHouseLanguageSaveGame* Loaded =
		Cast<UTheHouseLanguageSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, SaveUserIndex));
	if (!Loaded || Loaded->PreferredCultureName.IsEmpty())
	{
		return;
	}

	ApplyCultureInternal(Loaded->PreferredCultureName);
}
