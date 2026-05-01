#pragma once

#include "CoreMinimal.h"
#include "TheHouseGraphicsProfileTypes.h"

namespace TheHouseGraphicsProfile
{

THEHOUSE_API bool IsHighProfileAllowed();

THEHOUSE_API ETheHouseGraphicsProfile ResolveStartupProfile();

THEHOUSE_API FString GetProfileIniPath(ETheHouseGraphicsProfile Profile);

/** Applique les CVars du fichier INI du profil (chemin Config/). */
THEHOUSE_API void ApplyIniProfile(ETheHouseGraphicsProfile Profile);

} // namespace TheHouseGraphicsProfile
