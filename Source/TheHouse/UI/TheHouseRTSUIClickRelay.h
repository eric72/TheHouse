#pragma once

#include "CoreMinimal.h"
#include "TheHouseObject.h"
#include "UObject/Object.h"
#include "TheHouseRTSUIClickRelay.generated.h"

class ATheHousePlayerController;

UENUM()
enum class ETheHouseRTSUIClickKind : uint8
{
	Catalog,
	Stored,
	ContextOption
};

/**
 * Relais UObject pour brancher UButton::OnClicked (délégué dynamique) vers le PlayerController.
 */
UCLASS()
class THEHOUSE_API UTheHouseRTSUIClickRelay : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	ETheHouseRTSUIClickKind Kind = ETheHouseRTSUIClickKind::Catalog;

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> ContextTarget = nullptr;

	UPROPERTY(Transient)
	FName ContextOptionId;

	UPROPERTY(Transient)
	TSubclassOf<ATheHouseObject> CatalogClass;

	UPROPERTY(Transient)
	int32 StoredIndex = INDEX_NONE;

	UFUNCTION()
	void RelayClicked();
};
