#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TheHouseRTSMainWidget.generated.h"

class ATheHousePlayerController;
class UCanvasPanel;
class UScrollBox;
class UTextBlock;
class UTheHouseRTSUIClickRelay;

/**
 * Panneau RTS : argent + catalogue + stock (rempli en C++ ou via Widget Blueprint).
 *
 * Personnalisation (style Casino Inc, barre d’icônes, etc.) :
 * 1) BP_HouseController → Class Defaults → RTS Main Widget Class = ton WBP.
 * 2) Crée un Widget Blueprint hérité de cette classe.
 * 3) Dans le designer, nomme les widgets (BindWidgetOptional) :
 *    MoneyText, CatalogScroll, StoredScroll, et pour le personnel un UScrollBox nommé **StaffPaletteScroll**
 *    (ou **StaffListScroll**) n’importe où sous la racine / dans un sous-WBP : le C++ le retrouve même sans BindWidgetOptional.
 *    Le C++ y injecte texte / boutons. Tu peux les placer dans des onglets, WidgetSwitcher, etc.
 *    Style des lignes personnel : **Staff Palette Row Font Size** / **Staff Palette Row Text Color** (Class Defaults du WBP).
 *    Données recrutement (RandomDisplayNames, RandomGivenNames, etc.) : **NPC Staff Recruitment Pool** sur le **PlayerController** (BP), pas sur les Data Assets Staff.
 * 4) Implémente l’événement « After RTS Main Refreshed » pour synchroniser icônes / visibilité.
 *    Pour fermer des panneaux (catalogue/stock) sur Échap : surcharger « Escape Close Overlays » dans le WBP.
 *
 * Si tu ne poses pas ces trois noms, laisse RTS Main Widget Class sur la classe C++ pure
 * (pas de WBP enfant vide) : la hiérarchie par défaut est générée en code.
 */
UCLASS(Blueprintable)
class THEHOUSE_API UTheHouseRTSMainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void BindToPlayerController(ATheHousePlayerController* PC);
	void RefreshAll();

	UFUNCTION(BlueprintPure, Category="TheHouse|RTS|UI")
	ATheHousePlayerController* GetTheHouseOwnerPC() const { return OwnerPC; }

	/**
	 * Échap (via le PlayerController) : masquer panneaux / modales ajoutés dans le WBP.
	 * Retourner true si un panneau était ouvert et a été fermé (consomme la touche côté jeu).
	 */
	UFUNCTION(BlueprintNativeEvent, Category="TheHouse|RTS|UI", meta=(DisplayName="Escape Close Overlays"))
	bool EscapeCloseOverlays();
	virtual bool EscapeCloseOverlays_Implementation();

protected:
	virtual void NativeConstruct() override;

	/** Appelé après chaque RefreshAll (données catalogue / stock / argent à jour). */
	UFUNCTION(BlueprintImplementableEvent, Category="TheHouse|RTS|UI", meta=(DisplayName="After RTS Main Refreshed"))
	void BP_OnAfterRTSMainWidgetRefreshed();

	/** Optionnel : racine Canvas si tu veux l’utiliser depuis le BP. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	/** Obligatoires en mode designer (WBP) pour que le C++ remplisse listes et argent. */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UScrollBox> CatalogScroll = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UScrollBox> StoredScroll = nullptr;

	/** Liste des recrues / staff (clic = prévisualisation de pose, comme le catalogue d’objets). */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UScrollBox> StaffPaletteScroll = nullptr;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MoneyText = nullptr;

	/**
	 * Lignes du panneau Personnel (métiers, candidats, « ← Métiers ») : taille de police Slate.
	 * 0 = laisser la police par défaut du TextBlock.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|UI|Staff Palette", meta=(ClampMin="0", ClampMax="96", UIMin="0"))
	int32 StaffPaletteRowFontSize = 0;

	/** Couleur du texte pour ces mêmes lignes (défaut blanc, comme avant). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TheHouse|RTS|UI|Staff Palette")
	FSlateColor StaffPaletteRowTextColor = FSlateColor(FLinearColor::White);

private:
	void RebuildRootIfNeeded();
	void RebuildCatalogButtons();
	void RebuildStoredButtons();
	void RebuildStaffPaletteButtons();
	void RefreshMoney();

	UPROPERTY(Transient)
	TObjectPtr<ATheHousePlayerController> OwnerPC = nullptr;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTheHouseRTSUIClickRelay>> ClickRelays;
};
