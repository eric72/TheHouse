#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TheHouseGraphicsProfileTypes.h"
#include "TheHouseGraphicsProfileSubsystem.generated.h"

UCLASS()
class THEHOUSE_API UTheHouseGraphicsProfileSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "TheHouse|Graphics")
	ETheHouseGraphicsProfile GetActiveProfile() const;

	UFUNCTION(BlueprintCallable, Category = "TheHouse|Graphics")
	void SetProfile(ETheHouseGraphicsProfile Profile);

	/** Bascule Low <-> High (High ignoré en Shipping). */
	UFUNCTION(BlueprintCallable, Category = "TheHouse|Graphics")
	void ToggleProfile();

	UFUNCTION(BlueprintPure, Category = "TheHouse|Graphics")
	bool IsHighProfileAllowed() const;

	/** Synchronise le cache interne après application depuis UTheHouseGameUserSettings (sans sauvegarde disque). */
	void SetCachedGraphicsProfile(ETheHouseGraphicsProfile Profile);

private:
	ETheHouseGraphicsProfile ActiveProfile = ETheHouseGraphicsProfile::Low;
};
