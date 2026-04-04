// Fill out your copyright notice in the Description page of Project Settings.


#include "TheHouseGameModeBase.h"
#include "TheHouseCameraPawn.h"
#include "TheHousePlayerController.h"
#include "TheHouseHUD.h"
#include "GameFramework/PlayerController.h"

ATheHouseGameModeBase::ATheHouseGameModeBase()
{
	// Set default pawn class to the RTS Camera Pawn
	DefaultPawnClass = ATheHouseCameraPawn::StaticClass();

	// Set default player controller class
	PlayerControllerClass = ATheHousePlayerController::StaticClass();

	// Set default HUD class
	HUDClass = ATheHouseHUD::StaticClass();
}

void ATheHouseGameModeBase::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Très fréquent : un GameMode Blueprint laisse "Player Controller Class" sur la valeur Epic par défaut.
	// Dans ce cas aucun de nos correctifs d'entrée ne s'exécute — le jeu semble "ne jamais changer".
	const TSubclassOf<APlayerController> RequiredPC = ATheHousePlayerController::StaticClass();
	if (!PlayerControllerClass || !PlayerControllerClass->IsChildOf(RequiredPC))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[TheHouse] Mode de jeu « %s » : la classe de contrôleur joueur était « %s ». ")
			TEXT("Correction : ATheHousePlayerController. Vérifie aussi Paramètres du monde > Mode de jeu (remplacement) sur ta carte."),
			*GetClass()->GetName(),
			PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("nullptr"));
		PlayerControllerClass = RequiredPC;
	}
}

void ATheHouseGameModeBase::StartPlay()
{
	Super::StartPlay();
	UE_LOG(LogTemp, Warning, TEXT("[TheHouse] Mode de jeu actif : %s | Classe du contrôleur joueur : %s"),
		*GetClass()->GetName(),
		PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("nullptr"));
}
