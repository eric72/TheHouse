#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "TheHouseGraphicsProfileTypes.h"
#include "TheHouseGameUserSettings.generated.h"

/**
 * Préférences locales (graphismes, sons, etc.) : persistance standard UE
 * dans `Saved/Config/<Plateforme>/GameUserSettings.ini` (éditeur + jeu packagé).
 */
UCLASS()
class THEHOUSE_API UTheHouseGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UTheHouseGameUserSettings* GetTheHouseGameUserSettings();

	UFUNCTION(BlueprintPure, Category = "TheHouse|Settings")
	ETheHouseGraphicsProfile GetGraphicsQuality() const { return GraphicsQuality; }

	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void SetGraphicsQuality(ETheHouseGraphicsProfile InQuality);

	UFUNCTION(BlueprintPure, Category = "TheHouse|Settings")
	float GetMasterVolumeNormalized() const { return MasterVolumeNormalized; }

	UFUNCTION(BlueprintPure, Category = "TheHouse|Settings")
	float GetMusicVolumeNormalized() const { return MusicVolumeNormalized; }

	UFUNCTION(BlueprintPure, Category = "TheHouse|Settings")
	float GetSfxVolumeNormalized() const { return SfxVolumeNormalized; }

	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void SetMasterVolumeNormalized(float V);

	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void SetMusicVolumeNormalized(float V);

	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void SetSfxVolumeNormalized(float V);

	/** Sauvegarde disque + ligne d’audit (historique). */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void TheHouse_SaveToDisk();

	/** Sauvegarde `DefaultInput` / mappages (à appeler après remapping UI). */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void TheHouse_SaveInputMappings();

	/** Applique volumes (hook BP / Sound Mix côté projet) + notifie le sous-système graphique. */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|Settings")
	void TheHouse_ApplyToRuntime(UWorld* World);

	/** Migration one-shot depuis `Saved/TheHouseGraphicsUser.ini` (ancien système). */
	void MigrateLegacyGraphicsIniIfPresent();

	virtual void ApplySettings(bool bCheckForResolutionOverride) override;
	virtual void SaveSettings() override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "TheHouse|Settings", meta = (DisplayName = "Apply TheHouse Audio (BP)"))
	void BP_ApplyTheHouseAudioVolumes(float Master01, float Music01, float Sfx01);

private:
	UPROPERTY(Config)
	ETheHouseGraphicsProfile GraphicsQuality = ETheHouseGraphicsProfile::Low;

	UPROPERTY(Config)
	float MasterVolumeNormalized = 1.f;

	UPROPERTY(Config)
	float MusicVolumeNormalized = 1.f;

	UPROPERTY(Config)
	float SfxVolumeNormalized = 1.f;
};
