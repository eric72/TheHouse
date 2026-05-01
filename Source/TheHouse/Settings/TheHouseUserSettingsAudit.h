#pragma once

#include "CoreMinimal.h"

class UTheHouseGameUserSettings;

namespace TheHouseUserSettingsAudit
{

/** Append-only (UTF-8) : `Saved/TheHouse/settings_history.log`, une ligne par sauvegarde réussie. */
THEHOUSE_API void AppendSnapshotFromSettings(const UTheHouseGameUserSettings* Settings);

/** Appel explicite depuis l’UI quand les touches ont changé (après SaveConfig Input). */
THEHOUSE_API void AppendInputRemapNote(const FString& Note);

} // namespace TheHouseUserSettingsAudit
