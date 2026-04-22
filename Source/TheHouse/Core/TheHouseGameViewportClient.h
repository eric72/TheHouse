#pragma once

#include "CoreMinimal.h"
#include "Engine/GameViewportClient.h"
#include "TheHouseGameViewportClient.generated.h"

/**
 * Intercepte la molette au niveau de la vue jeu (avant les problèmes Game+UI / Slate).
 * Échap : fermeture RTS (menu, placement, sélection) avant que Slate ne consomme la touche.
 * Le clic droit (menu contextuel) est géré par le poll sur le PC (Game+UI mange souvent InputKey).
 */
UCLASS()
class THEHOUSE_API UTheHouseGameViewportClient : public UGameViewportClient
{
	GENERATED_BODY()

public:
	virtual bool InputAxis(const FInputKeyEventArgs& EventArgs) override;
	virtual bool InputKey(const FInputKeyEventArgs& EventArgs) override;
};
