#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TheHouseLocalizationSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTheHouseCultureChanged);

/**
 * Langue du jeu au runtime (équivalent conceptuel à i18n / locale Next.js).
 * Utilise la pile Internationalization du moteur : les FText / NSLOCTEXT suivent la culture courante
 * une fois les traductions compilées (Localization Dashboard).
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseLocalizationSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UTheHouseLocalizationSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Applique la culture et sauvegarde pour les prochains lancements. Retourne false si la culture est introuvable. */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|Localization")
	bool SetLanguage(const FString& CultureName);

	UFUNCTION(BlueprintPure, Category = "TheHouse|Localization")
	FString GetCurrentLanguage() const;

	/** Liste pour menus (boutons radio / liste). Valeurs du type « fr », « en » — à aligner avec tes locales compilées. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TheHouse|Localization")
	TArray<FString> SupportedCultureNames;

	UPROPERTY(BlueprintAssignable, Category = "TheHouse|Localization")
	FOnTheHouseCultureChanged OnCultureChanged;

protected:
	bool ApplyCultureInternal(const FString& CultureName);
	void PersistCulture(const FString& CultureName);
	void LoadPersistedCulture();

	static const TCHAR* SaveSlotName;
	static constexpr int32 SaveUserIndex = 0;
};
