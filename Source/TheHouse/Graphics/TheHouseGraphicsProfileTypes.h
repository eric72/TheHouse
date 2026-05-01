#pragma once

#include "CoreMinimal.h"

#include "TheHouseGraphicsProfileTypes.generated.h"

UENUM(BlueprintType)
enum class ETheHouseGraphicsProfile : uint8
{
	/** Steam Deck / GTX 1060 : Lumen off, CSM 2×1024, dynamic res. */
	Low UMETA(DisplayName = "Low (Production)"),
	/** Dev / captures : Lumen, CSM plus large, pas de budget dynamic res forcé. */
	High UMETA(DisplayName = "High (Dev/Capture)"),
};
