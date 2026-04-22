// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TheHouseGameModeBase.generated.h"

/**
 * 
 */
UCLASS()
class THEHOUSE_API ATheHouseGameModeBase : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	ATheHouseGameModeBase();

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
};
