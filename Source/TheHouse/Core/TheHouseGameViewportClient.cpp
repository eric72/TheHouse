#include "TheHouseGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "TheHousePlayerController.h"

bool UTheHouseGameViewportClient::InputAxis(const FInputKeyEventArgs& EventArgs)
{
	// En Game+UI la molette n’atteint souvent pas BindAxis / PlayerInput : on applique ici avant Super.
	if (EventArgs.Key == EKeys::MouseWheelAxis && !FMath::IsNearlyZero(EventArgs.AmountDepressed))
	{
		ATheHousePlayerController* THPC = nullptr;

		if (GEngine)
		{
			ULocalPlayer* LP = GEngine->GetLocalPlayerFromInputDevice(this, EventArgs.InputDevice);
			if (!LP)
			{
				const TArray<ULocalPlayer*>& LPs = GEngine->GetGamePlayers(this);
				if (LPs.Num() == 1)
				{
					LP = LPs[0];
				}
				else
				{
					for (ULocalPlayer* Candidate : LPs)
					{
						if (Candidate && Cast<ATheHousePlayerController>(Candidate->PlayerController))
						{
							LP = Candidate;
							break;
						}
					}
				}
			}

			if (LP)
			{
				THPC = Cast<ATheHousePlayerController>(LP->PlayerController);
			}

			if (!THPC && GEngine->GameViewport)
			{
				if (UWorld* ViewportWorld = GEngine->GameViewport->GetWorld())
				{
					if (APlayerController* PC = ViewportWorld->GetFirstPlayerController())
					{
						THPC = Cast<ATheHousePlayerController>(PC);
					}
				}
			}

			if (THPC)
			{
				THPC->TheHouse_ApplyWheelZoom(EventArgs.AmountDepressed);
				// Ne pas appeler Super : sinon PlayerInput retaxe aussi l’axe Zoom → double zoom / sensation « cassée ».
				return true;
			}
		}
	}

	return Super::InputAxis(EventArgs);
}
