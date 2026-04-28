#include "TheHouse/NPC/Persistence/TheHouseNPCPersistenceSubsystem.h"

#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouse/NPC/TheHouseNPCIdentity.h"
#include "TheHouse/NPC/Persistence/TheHouseNPCPersistenceSaveGame.h"
#include "TheHouse/NPC/Spawning/TheHouseNPCSpawnSubsystem.h"
#include "TheHouse/NPC/Spawning/TheHouseNPCSpawnPoint.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

static FName TheHouse_GetCurrentLevelNameSafe(const UWorld* World)
{
	if (!World)
	{
		return NAME_None;
	}
	return FName(*UGameplayStatics::GetCurrentLevelName(World, /*bRemovePrefixString*/ true));
}

void UTheHouseNPCPersistenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Seed par défaut : stable pour la session, et sauvegardable.
	if (RandomSeed == 0)
	{
		RandomSeed = (int32)(FDateTime::UtcNow().ToUnixTimestamp() & 0x7fffffff);
		if (RandomSeed == 0)
		{
			RandomSeed = 1337;
		}
	}
}

void UTheHouseNPCPersistenceSubsystem::Deinitialize()
{
	States.Reset();
	LiveActorToId.Reset();
	LastLiveSnapshotSeconds = -1.f;
	Super::Deinitialize();
}

TStatId UTheHouseNPCPersistenceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTheHouseNPCPersistenceSubsystem, STATGROUP_Tickables);
}

float UTheHouseNPCPersistenceSubsystem::GetNowSeconds() const
{
	// GameInstanceSubsystem vit au-delà des mondes : on préfère un temps monotone local.
	return (float)FPlatformTime::Seconds();
}

void UTheHouseNPCPersistenceSubsystem::Tick(float DeltaTime)
{
	const float Now = GetNowSeconds();

	// Snapshot léger des PNJ instanciés (pour sauvegarde / cohérence).
	if (UWorld* World = GetWorld())
	{
		if (LastLiveSnapshotSeconds < 0.f || (Now - LastLiveSnapshotSeconds) >= 0.5f)
		{
			SnapshotLiveNPCsIntoStates(World);
			LastLiveSnapshotSeconds = Now;
		}
	}

	// 1) Simuler les états qui ne sont pas instanciés.
	for (auto& Pair : States)
	{
		FTheHouseNPCPersistentState& S = Pair.Value;
		if (S.Presence == ETheHouseNPCWorldPresence::Simulated)
		{
			SimulateState(S, Now);
		}
	}

	// 2) Opportuniste : si un monde est actif et que certains PNJ “devraient” être visibles, respawn.
	// (Minimal : on respawn seulement ceux inside/outside d’un bâtiment si le niveau courant contient des spawn points adaptés.)
	for (auto& Pair : States)
	{
		TryRespawnIfRelevant(Pair.Value);
	}
}

void UTheHouseNPCPersistenceSubsystem::EnsureStateForActor(ATheHouseNPCCharacter* NPC)
{
	if (!IsValid(NPC))
	{
		return;
	}

	if (!NPC->PersistentId.IsValid())
	{
		NPC->SetPersistentId(FGuid::NewGuid());
	}

	if (!States.Contains(NPC->PersistentId))
	{
		FTheHouseNPCPersistentState S;
		S.Id = NPC->PersistentId;
		S.Place = ETheHouseNPCPlace::Outside;
		S.BuildingId = NAME_None;
		S.WorldSlot = ETheHouseNPCWorldSlot::City;
		if (UWorld* W = NPC->GetWorld())
		{
			S.LastLevelName = TheHouse_GetCurrentLevelNameSafe(W);
		}
		S.NPCClass = NPC->GetClass();
		S.Category = ITheHouseNPCIdentity::Execute_GetNPCCategory(NPC);
		S.Presence = ETheHouseNPCWorldPresence::Spawned;
		S.NextDecisionTimeSeconds = GetNowSeconds() + FMath::FRandRange(3.f, 12.f);
		S.SimRole = NAME_None;
		CopyActorToState(NPC, S);
		States.Add(S.Id, S);
	}
}

void UTheHouseNPCPersistenceSubsystem::RegisterSpawnedNPC(ATheHouseNPCCharacter* NPC)
{
	if (!IsValid(NPC))
	{
		return;
	}

	EnsureStateForActor(NPC);

	FTheHouseNPCPersistentState& S = States.FindChecked(NPC->PersistentId);
	S.Presence = ETheHouseNPCWorldPresence::Spawned;
	S.NPCClass = NPC->GetClass();
	S.Category = ITheHouseNPCIdentity::Execute_GetNPCCategory(NPC);
	if (UWorld* W = NPC->GetWorld())
	{
		S.LastLevelName = TheHouse_GetCurrentLevelNameSafe(W);
	}
	CopyActorToState(NPC, S);

	// Important : éviter les doublons de WeakPtr dans la map (sinon snapshot + TryRespawn peuvent “voir” plusieurs fois le même PNJ).
	LiveActorToId.FindOrAdd(NPC) = NPC->PersistentId;
}

void UTheHouseNPCPersistenceSubsystem::NotifyNPCDespawned(ATheHouseNPCCharacter* NPC)
{
	if (!IsValid(NPC))
	{
		return;
	}

	if (!NPC->PersistentId.IsValid())
	{
		return;
	}

	if (FTheHouseNPCPersistentState* S = States.Find(NPC->PersistentId))
	{
		S->Presence = ETheHouseNPCWorldPresence::Simulated;
		S->NextDecisionTimeSeconds = GetNowSeconds() + FMath::FRandRange(2.f, 10.f);
		CopyActorToState(NPC, *S);
	}

	LiveActorToId.Remove(NPC);
}

void UTheHouseNPCPersistenceSubsystem::CopyActorToState(ATheHouseNPCCharacter* NPC, FTheHouseNPCPersistentState& Out) const
{
	if (!IsValid(NPC))
	{
		return;
	}

	Out.LastKnownTransform = NPC->GetActorTransform();
	Out.bHasLastKnownTransform = true;

	Out.DisplayName = NPC->StaffIdentityDisplayName;
	Out.Wallet = NPC->Wallet;
	Out.StaffMonthlySalary = NPC->StaffMonthlySalary;
	Out.StaffStarRating = NPC->StaffStarRating;
	Out.StaffEmploymentStartInGameSeconds = NPC->StaffEmploymentStartInGameSeconds;
	Out.StaffLastSalaryPaidDayIndex = NPC->StaffLastSalaryPaidDayIndex;
	Out.StaffMeshVariantRollIndex = NPC->StaffMeshVariantRollIndex;

	// Action en cours (définie par l’IA / BP).
	{
		FName ActionId = NAME_None;
		float Remaining = 0.f;
		bool bHasTarget = false;
		FVector Target = FVector::ZeroVector;
		NPC->GetPersistedCurrentActionSnapshot(ActionId, Remaining, bHasTarget, Target);
		Out.CurrentActionId = ActionId;
		Out.CurrentActionRemainingSeconds = FMath::Max(0.f, Remaining);
		Out.bHasCurrentActionTargetWorldLocation = bHasTarget;
		Out.CurrentActionTargetWorldLocation = Target;
	}
}

bool UTheHouseNPCPersistenceSubsystem::ShouldExistInCurrentWorld(const UWorld* World, const FTheHouseNPCPersistentState& S) const
{
	if (!World)
	{
		return false;
	}

	const FName Current = TheHouse_GetCurrentLevelNameSafe(World);
	if (S.LastLevelName != NAME_None && Current != NAME_None && S.LastLevelName != Current)
	{
		return false;
	}

	// Si on a un slot “intérieur”, on ne respawn pas dans la ville (et inversement si tu tagges explicitement).
	if (S.WorldSlot == ETheHouseNPCWorldSlot::Interior)
	{
		// Heuristique : le niveau intérieur doit matcher le nom sauvegardé ; sinon on attend le bon niveau.
		return S.LastLevelName == Current;
	}

	// City : si LastLevelName est vide, on accepte (migration) ; sinon match strict.
	if (S.LastLevelName == NAME_None)
	{
		return true;
	}
	return S.LastLevelName == Current;
}

void UTheHouseNPCPersistenceSubsystem::SimulateState(FTheHouseNPCPersistentState& S, float NowSeconds)
{
	if (S.NextDecisionTimeSeconds > NowSeconds)
	{
		return;
	}

	const uint32 Seed = (uint32)(RandomSeed ^ S.Id.A ^ S.Id.B);
	FRandomStream Rng(Seed);

	// Simulation simple (extensible) :
	// - Clients inside casino : alternent “Playing”/“Wandering” et parfois sortent.
	// - Clients outside : parfois entrent dans leur BuildingId si défini.
	const bool bInBuilding = (S.Place == ETheHouseNPCPlace::InsideBuilding) && (S.BuildingId != NAME_None);

	if (bInBuilding)
	{
		const float Roll = Rng.FRand();
		if (Roll < 0.65f)
		{
			S.SimRole = FName(TEXT("Playing"));
		}
		else if (Roll < 0.90f)
		{
			S.SimRole = FName(TEXT("Wandering"));
		}
		else
		{
			S.SimRole = FName(TEXT("Leaving"));
			S.Place = ETheHouseNPCPlace::Outside;
		}
	}
	else
	{
		S.SimRole = FName(TEXT("Outside"));
		if (S.BuildingId != NAME_None && Rng.FRand() < 0.25f)
		{
			S.Place = ETheHouseNPCPlace::InsideBuilding;
			S.SimRole = FName(TEXT("Entering"));
		}
	}

	S.NextDecisionTimeSeconds = NowSeconds + Rng.FRandRange(4.f, 15.f);
}

void UTheHouseNPCPersistenceSubsystem::TryRespawnIfRelevant(FTheHouseNPCPersistentState& S)
{
	if (S.Presence != ETheHouseNPCWorldPresence::Simulated)
	{
		return;
	}

	// Si un acteur live existe déjà pour cet Id (cas doublons LiveActorToId / états incohérents), ne pas respawn.
	for (const auto& Pair : LiveActorToId)
	{
		if (Pair.Value == S.Id)
		{
			return;
		}
	}

	// On ne respawn pas sans classe.
	if (!S.NPCClass.IsValid() && S.NPCClass.ToSoftObjectPath().IsNull())
	{
		return;
	}

	UWorld* World = nullptr;
	if (UGameInstance* GI = GetGameInstance())
	{
		World = GI->GetWorld();
	}
	if (!World)
	{
		return;
	}

	if (!ShouldExistInCurrentWorld(World, S))
	{
		return;
	}

	UClass* Cls = S.NPCClass.LoadSynchronous();
	TSubclassOf<ATheHouseNPCCharacter> NPCClass = Cls;
	if (!NPCClass)
	{
		return;
	}

	FTransform SpawnXform = S.LastKnownTransform;
	if (!S.bHasLastKnownTransform)
	{
		// Fallback : spawn casino random si des points existent.
		if (UTheHouseNPCSpawnSubsystem* SpawnSys = World->GetSubsystem<UTheHouseNPCSpawnSubsystem>())
		{
			const TArray<ATheHouseNPCCharacter*> Spawned = SpawnSys->SpawnNPCs_CasinoRandom(NPCClass, 1);
			if (Spawned.Num() > 0 && IsValid(Spawned[0]))
			{
				ATheHouseNPCCharacter* N = Spawned[0];
				N->SetPersistentId(S.Id);
				S.Presence = ETheHouseNPCWorldPresence::Spawned;
				LiveActorToId.FindOrAdd(N) = S.Id;
			}
		}
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	ATheHouseNPCCharacter* N = World->SpawnActor<ATheHouseNPCCharacter>(NPCClass, SpawnXform, Params);
	if (!IsValid(N))
	{
		return;
	}

	N->SetPersistentId(S.Id);
	S.Presence = ETheHouseNPCWorldPresence::Spawned;
	LiveActorToId.FindOrAdd(N) = S.Id;
}

void UTheHouseNPCPersistenceSubsystem::MoveNPCToBuilding(
	ATheHouseNPCCharacter* NPC,
	const FName BuildingId,
	const bool bInside,
	const bool bTeleportIfPossible)
{
	if (!IsValid(NPC))
	{
		return;
	}

	RegisterSpawnedNPC(NPC);

	FTheHouseNPCPersistentState& S = States.FindChecked(NPC->PersistentId);
	S.BuildingId = BuildingId;
	S.Place = bInside ? ETheHouseNPCPlace::InsideBuilding : ETheHouseNPCPlace::Outside;
	S.WorldSlot = bInside ? ETheHouseNPCWorldSlot::Interior : ETheHouseNPCWorldSlot::City;
	if (UWorld* W = NPC->GetWorld())
	{
		S.LastLevelName = TheHouse_GetCurrentLevelNameSafe(W);
	}
	CopyActorToState(NPC, S);

	// Téléport si possible = même monde + le gameplay/blueprints appelle explicitement TeleportTo.
	// Ici on ne connaît pas encore les points intérieur/extérieur du casino (acteur portail) : on bascule juste l’état.
	if (!bTeleportIfPossible)
	{
		NotifyNPCDespawned(NPC);
		NPC->Destroy();
	}
}

void UTheHouseNPCPersistenceSubsystem::SpawnSimulatedCasinoVisitors(
	const FName BuildingId,
	TSubclassOf<ATheHouseNPCCharacter> NPCClass,
	int32 Count)
{
	Count = FMath::Max(0, Count);
	if (!NPCClass || Count == 0)
	{
		return;
	}

	const float Now = GetNowSeconds();

	for (int32 i = 0; i < Count; ++i)
	{
		FTheHouseNPCPersistentState S;
		S.Id = FGuid::NewGuid();
		S.BuildingId = BuildingId;
		S.Place = ETheHouseNPCPlace::InsideBuilding;
		S.WorldSlot = ETheHouseNPCWorldSlot::Interior;
		if (UWorld* W = GetWorld())
		{
			S.LastLevelName = TheHouse_GetCurrentLevelNameSafe(W);
		}
		S.NPCClass = NPCClass;
		S.Category = ETheHouseNPCCategory::Customer;
		S.Presence = ETheHouseNPCWorldPresence::Simulated;
		S.NextDecisionTimeSeconds = Now + FMath::FRandRange(2.f, 10.f);
		S.SimRole = FName(TEXT("Entering"));
		States.Add(S.Id, S);
	}
}

void UTheHouseNPCPersistenceSubsystem::GetAllNPCStates(TArray<FTheHouseNPCPersistentState>& OutStates) const
{
	OutStates.Reset();
	OutStates.Reserve(States.Num());
	for (const auto& Pair : States)
	{
		OutStates.Add(Pair.Value);
	}
}

bool UTheHouseNPCPersistenceSubsystem::SaveToSlot(const FString& SlotName, const int32 UserIndex)
{
	UTheHouseNPCPersistenceSaveGame* Save =
		Cast<UTheHouseNPCPersistenceSaveGame>(UGameplayStatics::CreateSaveGameObject(UTheHouseNPCPersistenceSaveGame::StaticClass()));
	if (!Save)
	{
		return false;
	}

	Save->SavedStates.Reserve(States.Num());
	for (const auto& Pair : States)
	{
		Save->SavedStates.Add(Pair.Value);
	}
	Save->RandomSeed = RandomSeed;

	return UGameplayStatics::SaveGameToSlot(Save, SlotName, UserIndex);
}

bool UTheHouseNPCPersistenceSubsystem::LoadFromSlot(const FString& SlotName, const int32 UserIndex)
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
	{
		return false;
	}

	UTheHouseNPCPersistenceSaveGame* Loaded =
		Cast<UTheHouseNPCPersistenceSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
	if (!Loaded)
	{
		return false;
	}

	// On charge les tickets. Les acteurs live du monde courant restent live, mais peuvent être “réalignés”
	// via RegisterSpawnedNPC lors de leur BeginPlay / initialisation de niveau.
	States.Reset();
	for (const FTheHouseNPCPersistentState& S : Loaded->SavedStates)
	{
		if (S.Id.IsValid())
		{
			States.Add(S.Id, S);
		}
	}
	RandomSeed = Loaded->RandomSeed;
	if (RandomSeed == 0)
	{
		RandomSeed = 1337;
	}

	return true;
}

void UTheHouseNPCPersistenceSubsystem::ApplyLoadedStates(const TArray<FTheHouseNPCPersistentState>& InStates, const int32 InRandomSeed)
{
	States.Reset();
	LiveActorToId.Reset();

	for (const FTheHouseNPCPersistentState& S : InStates)
	{
		if (S.Id.IsValid())
		{
			States.Add(S.Id, S);
		}
	}

	RandomSeed = InRandomSeed;
	if (RandomSeed == 0)
	{
		RandomSeed = 1337;
	}
}

void UTheHouseNPCPersistenceSubsystem::SnapshotLiveNPCsIntoStates(UWorld* World)
{
	if (!World)
	{
		return;
	}

	for (const auto& Pair : LiveActorToId)
	{
		if (ATheHouseNPCCharacter* N = Pair.Key.Get())
		{
			if (FTheHouseNPCPersistentState* S = States.Find(Pair.Value))
			{
				S->Presence = ETheHouseNPCWorldPresence::Spawned;
				S->NPCClass = N->GetClass();
				S->Category = ITheHouseNPCIdentity::Execute_GetNPCCategory(N);
				S->LastLevelName = TheHouse_GetCurrentLevelNameSafe(World);
				CopyActorToState(N, *S);
			}
		}
	}
}

void UTheHouseNPCPersistenceSubsystem::RespawnNPCsForCurrentWorld(UWorld* World)
{
	if (!World)
	{
		return;
	}

	// Détruire tous les PNJ du monde courant (on les recrée depuis l’état).
	TArray<ATheHouseNPCCharacter*> ToKill;
	for (TActorIterator<ATheHouseNPCCharacter> It(World); It; ++It)
	{
		if (ATheHouseNPCCharacter* N = *It)
		{
			ToKill.Add(N);
		}
	}
	for (ATheHouseNPCCharacter* N : ToKill)
	{
		if (IsValid(N))
		{
			NotifyNPCDespawned(N);
			N->Destroy();
		}
	}
	LiveActorToId.Reset();

	for (auto& Pair : States)
	{
		FTheHouseNPCPersistentState& S = Pair.Value;
		if (!ShouldExistInCurrentWorld(World, S))
		{
			S.Presence = ETheHouseNPCWorldPresence::Simulated;
			continue;
		}

		UClass* Cls = S.NPCClass.LoadSynchronous();
		TSubclassOf<ATheHouseNPCCharacter> NPCClass = Cls;
		if (!NPCClass)
		{
			continue;
		}

		FTransform Xf = S.LastKnownTransform;
		if (!S.bHasLastKnownTransform)
		{
			// Sans transform, on laisse en simulated jusqu’à ce qu’un système de spawn fournisse une pose.
			S.Presence = ETheHouseNPCWorldPresence::Simulated;
			continue;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		ATheHouseNPCCharacter* N = World->SpawnActor<ATheHouseNPCCharacter>(NPCClass, Xf, Params);
		if (!IsValid(N))
		{
			continue;
		}

		N->SetPersistentId(S.Id);
		S.Presence = ETheHouseNPCWorldPresence::Spawned;
		LiveActorToId.FindOrAdd(N) = S.Id;
	}
}

bool UTheHouseNPCPersistenceSubsystem::TryGetStateCopy(const FGuid& Id, FTheHouseNPCPersistentState& OutState) const
{
	if (!Id.IsValid())
	{
		return false;
	}
	if (const FTheHouseNPCPersistentState* Found = States.Find(Id))
	{
		OutState = *Found;
		return true;
	}
	return false;
}

