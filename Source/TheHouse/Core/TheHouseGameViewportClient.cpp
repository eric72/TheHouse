#include "TheHouseGameViewportClient.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "TheHousePlayerController.h"

namespace TheHouseViewportInput
{
static ATheHousePlayerController* ResolveTheHousePC(UGameViewportClient* ViewportClient,
													FInputDeviceId InputDevice)
{
	if (!GEngine || !ViewportClient)
	{
		return nullptr;
	}

	ULocalPlayer* LP = GEngine->GetLocalPlayerFromInputDevice(ViewportClient, InputDevice);
	if (!LP)
	{
		const TArray<ULocalPlayer*>& LPs = GEngine->GetGamePlayers(ViewportClient);
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
		if (ATheHousePlayerController* THPC = Cast<ATheHousePlayerController>(LP->PlayerController))
		{
			return THPC;
		}
	}

	if (GEngine->GameViewport)
	{
		if (UWorld* ViewportWorld = GEngine->GameViewport->GetWorld())
		{
			if (APlayerController* PC = ViewportWorld->GetFirstPlayerController())
			{
				return Cast<ATheHousePlayerController>(PC);
			}
		}
	}

	return nullptr;
}
} // namespace TheHouseViewportInput

bool UTheHouseGameViewportClient::InputAxis(const FInputKeyEventArgs& EventArgs)
{
	// En Game+UI la molette n’atteint souvent pas BindAxis / PlayerInput : on applique ici avant Super.
	if (EventArgs.Key == EKeys::MouseWheelAxis && !FMath::IsNearlyZero(EventArgs.AmountDepressed))
	{
		if (ATheHousePlayerController* THPC =
				TheHouseViewportInput::ResolveTheHousePC(this, EventArgs.InputDevice))
		{
			THPC->TheHouse_ApplyWheelZoom(EventArgs.AmountDepressed);
			// Ne pas appeler Super : sinon PlayerInput retaxe aussi l’axe Zoom → double zoom / sensation « cassée ».
			return true;
		}
	}

	return Super::InputAxis(EventArgs);
}
