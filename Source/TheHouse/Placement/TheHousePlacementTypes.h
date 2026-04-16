#pragma once

#include "CoreMinimal.h"
#include "TheHousePlacementTypes.generated.h"

UENUM(BlueprintType)
enum class EPlacementState : uint8
{
    None UMETA(DisplayName = "None"),
    Previewing UMETA(DisplayName = "Previewing"),
    Placing UMETA(DisplayName = "Placing")
};