#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TheHouseSelectable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTheHouseSelectable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for actors that can be selected by the player.
 */
class THEHOUSE_API ITheHouseSelectable
{
	GENERATED_BODY()

public:
	/** Called when the object is selected */
	virtual void OnSelect() = 0;

	/** Called when the object is deselected */
	virtual void OnDeselect() = 0;

	/** Returns true if this object can be selected */
	virtual bool IsSelectable() const { return true; }
};
