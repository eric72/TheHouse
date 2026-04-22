#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseNPCOrderContextMenuUMGWidget.generated.h"

class ATheHouseNPCCharacter;
class ATheHouseObject;
class ATheHousePlayerController;
class UBorder;
class UVerticalBox;
class UTheHouseRTSUIClickRelay;

/** Cible du menu « ordres PNJ » (clic droit avec au moins un PNJ sélectionné au gauche). */
UENUM(BlueprintType)
enum class ETheHouseNPCOrderMenuTargetKind : uint8
{
	Object UMETA(DisplayName = "Objet placé"),
	NPC UMETA(DisplayName = "Autre PNJ"),
	Ground UMETA(DisplayName = "Sol"),
};

/**
 * Menu contextuel d’ordres : PNJ(s) déjà sélectionné(s) au clic gauche, clic droit sur un objet ou un autre PNJ.
 * Les lignes viennent de GatherNPCOrderActionsOnObject / GatherNPCOrderActionsOnNPC sur le PlayerController.
 * Dérivez un Widget Blueprint de cette classe pour le layout (BindWidgetOptional : MenuBorder, OptionsBox).
 */
UCLASS(Blueprintable)
class THEHOUSE_API UTheHouseNPCOrderContextMenuUMGWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI|NPC|Orders")
	void OpenForOrderOnObject(ATheHousePlayerController* InOwnerPC, ATheHouseObject* InTargetObject, const FVector2D& ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI|NPC|Orders")
	void OpenForOrderOnNPC(ATheHousePlayerController* InOwnerPC, ATheHouseNPCCharacter* InTargetNPC, const FVector2D& ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "TheHouse|RTS|UI|NPC|Orders")
	void OpenForOrderOnGround(ATheHousePlayerController* InOwnerPC, const FVector& InTargetLocation, const FVector2D& ScreenPosition);

	UFUNCTION(BlueprintPure, Category = "TheHouse|RTS|UI|NPC|Orders")
	ETheHouseNPCOrderMenuTargetKind GetOrderTargetKind() const { return TargetKind; }

	UFUNCTION(BlueprintPure, Category = "TheHouse|RTS|UI|NPC|Orders")
	ATheHouseObject* GetTargetObject() const { return TargetObject.Get(); }

	UFUNCTION(BlueprintPure, Category = "TheHouse|RTS|UI|NPC|Orders")
	ATheHouseNPCCharacter* GetTargetNPC() const { return TargetNPC.Get(); }

	UFUNCTION(BlueprintPure, Category = "TheHouse|RTS|UI|NPC|Orders")
	FVector GetTargetGroundLocation() const { return TargetGroundLocation; }

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> MenuBorder = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> OptionsBox = nullptr;

private:
	void EnsureRuntimeOptionsBox();
	void RebuildOptionButtons();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	ETheHouseNPCOrderMenuTargetKind TargetKind = ETheHouseNPCOrderMenuTargetKind::Object;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseObject> TargetObject = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<ATheHouseNPCCharacter> TargetNPC = nullptr;

	UPROPERTY(Transient)
	FVector TargetGroundLocation = FVector::ZeroVector;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTheHouseRTSUIClickRelay>> ClickRelays;
};
