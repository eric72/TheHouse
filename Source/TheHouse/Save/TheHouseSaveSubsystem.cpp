#include "TheHouse/Save/TheHouseSaveSubsystem.h"

#include "TheHouse/Save/TheHouseSaveGame.h"
#include "TheHouse/Player/TheHousePlayerController.h"
#include "TheHouse/Object/TheHouseObject.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouse/NPC/Persistence/TheHouseNPCPersistenceSubsystem.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ATheHousePlayerController* UTheHouseSaveSubsystem::GetHousePC(UWorld* World) const
{
	if (!World)
	{
		return nullptr;
	}
	return Cast<ATheHousePlayerController>(UGameplayStatics::GetPlayerController(World, 0));
}

bool UTheHouseSaveSubsystem::SaveAll(const FString& SlotName, const int32 UserIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UTheHouseSaveGame* Save = Cast<UTheHouseSaveGame>(UGameplayStatics::CreateSaveGameObject(UTheHouseSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	// --- Player ---
	if (ATheHousePlayerController* PC = GetHousePC(World))
	{
		Save->Money = PC->GetMoney();
		Save->InGameTimeSeconds = PC->GetInGameTimeSeconds();
		Save->InGameDayIndex = PC->GetInGameDayIndex();
		Save->StoredPlaceableStacks = PC->GetStoredPlaceableStacks();
	}

	// --- Placed objects (world scan) ---
	for (TActorIterator<ATheHouseObject> It(World); It; ++It)
	{
		ATheHouseObject* Obj = *It;
		if (!IsValid(Obj))
		{
			continue;
		}

		FTheHouseSavedPlacedObject SO;
		SO.ObjectClass = Obj->GetClass();
		SO.WorldTransform = Obj->GetActorTransform();
		SO.bNpcMoneyFlowEnabled = Obj->bNpcMoneyFlowEnabled;
		SO.NpcSpendToPlay = Obj->NpcSpendToPlay;
		SO.NpcMaxReturnToHouse = Obj->NpcMaxReturnToHouse;
		Save->PlacedObjects.Add(SO);
	}

	// --- NPC persistent states ---
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTheHouseNPCPersistenceSubsystem* Persist = GI->GetSubsystem<UTheHouseNPCPersistenceSubsystem>())
		{
			Persist->SnapshotLiveNPCsIntoStates(World);
			Persist->GetAllNPCStates(Save->NPCStates);
			Save->NPCRandomSeed = Persist->GetRandomSeed();
		}
	}

	return UGameplayStatics::SaveGameToSlot(Save, SlotName, UserIndex);
}

bool UTheHouseSaveSubsystem::LoadAll(const FString& SlotName, const int32 UserIndex)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	UTheHouseSaveGame* Loaded = Cast<UTheHouseSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!Loaded)
	{
		return false;
	}

	// --- Player economy/inventory ---
	if (ATheHousePlayerController* PC = GetHousePC(World))
	{
		PC->SetMoney(Loaded->Money);
		PC->SetInGameClockState(Loaded->InGameTimeSeconds, Loaded->InGameDayIndex);
		PC->SetStoredPlaceableStacks(Loaded->StoredPlaceableStacks);
	}

	// --- World objects ---
	// Stratégie simple : détruire les objets existants puis respawn ceux de la save.
	// (On pourra affiner: diff, ids persistants d’objets, etc.)
	TArray<ATheHouseObject*> Existing;
	for (TActorIterator<ATheHouseObject> It(World); It; ++It)
	{
		if (ATheHouseObject* Obj = *It)
		{
			Existing.Add(Obj);
		}
	}
	for (ATheHouseObject* Obj : Existing)
	{
		if (IsValid(Obj))
		{
			Obj->Destroy();
		}
	}

	for (const FTheHouseSavedPlacedObject& SO : Loaded->PlacedObjects)
	{
		UClass* Cls = SO.ObjectClass.LoadSynchronous();
		if (!Cls)
		{
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ATheHouseObject* Obj = World->SpawnActor<ATheHouseObject>(Cls, SO.WorldTransform, Params);
		if (!Obj)
		{
			continue;
		}

		Obj->bNpcMoneyFlowEnabled = SO.bNpcMoneyFlowEnabled;
		Obj->NpcSpendToPlay = SO.NpcSpendToPlay;
		Obj->NpcMaxReturnToHouse = SO.NpcMaxReturnToHouse;
	}

	// --- NPC states ---
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTheHouseNPCPersistenceSubsystem* Persist = GI->GetSubsystem<UTheHouseNPCPersistenceSubsystem>())
		{
			Persist->ApplyLoadedStates(Loaded->NPCStates, Loaded->NPCRandomSeed);
			Persist->RespawnNPCsForCurrentWorld(World);
		}
	}

	return true;
}

