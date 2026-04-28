#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TheHouse/NPC/TheHouseNPCTypes.h"
#include "TheHouseNPCPersistenceSubsystem.generated.h"

class ATheHouseNPCCharacter;
class APlayerController;
class UTheHouseNPCSpawnSubsystem;

UENUM(BlueprintType)
enum class ETheHouseNPCWorldPresence : uint8
{
	/** L’acteur est instancié dans le monde courant. */
	Spawned,
	/** L’acteur n’est pas instancié : on simule son état (hors-champ / autre niveau). */
	Simulated,
};

UENUM(BlueprintType)
enum class ETheHouseNPCPlace : uint8
{
	Outside,
	InsideBuilding,
};

/** Niveau / “monde logique” où le PNJ doit exister (pour respawn après OpenLevel). */
UENUM(BlueprintType)
enum class ETheHouseNPCWorldSlot : uint8
{
	/** Ville / map principale (hors intérieur casino). */
	City UMETA(DisplayName = "City"),

	/** Intérieur d’un bâtiment (ex: casino). */
	Interior UMETA(DisplayName = "Interior"),
};

USTRUCT(BlueprintType)
struct FTheHouseNPCPersistentState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	FGuid Id;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	ETheHouseNPCPlace Place = ETheHouseNPCPlace::Outside;

	/** Identifiant du bâtiment (ex: Casino_01). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	FName BuildingId = NAME_None;

	/** Slot monde : ville vs intérieur (casino). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	ETheHouseNPCWorldSlot WorldSlot = ETheHouseNPCWorldSlot::City;

	/** Nom de niveau UE au moment du dernier snapshot (`UGameplayStatics::GetCurrentLevelName`). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	FName LastLevelName = NAME_None;

	/** Classe d’acteur à respawn quand nécessaire. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	TSoftClassPtr<ATheHouseNPCCharacter> NPCClass;

	/** Catégorie PNJ (staff/client) — utile pour restaurer l’économie / payroll. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	ETheHouseNPCCategory Category = ETheHouseNPCCategory::Unassigned;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	ETheHouseNPCWorldPresence Presence = ETheHouseNPCWorldPresence::Simulated;

	/** Dernière pose connue (monde) pour ce PNJ. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	FTransform LastKnownTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	bool bHasLastKnownTransform = false;

	// --- Runtime fields (staff / customer) ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	float Wallet = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	float StaffMonthlySalary = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	int32 StaffStarRating = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	float StaffEmploymentStartInGameSeconds = 0.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	int32 StaffLastSalaryPaidDayIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	int32 StaffMeshVariantRollIndex = INDEX_NONE;

	// --- Action en cours (pour reprise après Load) ---
	/** Identifiant d’action “courante” (ex: PlayingSlot, WalkingToEntrance, WorkingAtMachine). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence|Action")
	FName CurrentActionId = NAME_None;

	/** Temps restant (secondes réelles) pour finir l’action courante au moment du snapshot/save. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence|Action")
	float CurrentActionRemainingSeconds = 0.f;

	/** Cible libre (optionnel) : position monde liée à l’action (ex: destination). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence|Action")
	FVector CurrentActionTargetWorldLocation = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence|Action")
	bool bHasCurrentActionTargetWorldLocation = false;

	/** “Vie” simulée : prochain changement de comportement (sec). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	float NextDecisionTimeSeconds = 0.f;

	/** Debug simple : “Playing”, “Wandering”, “Working”… */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Persistence")
	FName SimRole = NAME_None;
};

/**
 * Persistance PNJ inter-niveaux : garde un état qui continue d’évoluer même si les PNJ ne sont pas instanciés.
 * On respawn les acteurs quand le monde (extérieur / intérieur casino) est chargé et qu’ils doivent être visibles.
 */
UCLASS(BlueprintType)
class THEHOUSE_API UTheHouseNPCPersistenceSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// --- Tick ---
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return true; }

	/** Enregistre un acteur PNJ instancié comme “persistant” (création d’état si absent). */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence")
	void RegisterSpawnedNPC(ATheHouseNPCCharacter* NPC);

	/** Notifie que l’acteur PNJ est retiré du monde (on garde l’état simulé). */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence")
	void NotifyNPCDespawned(ATheHouseNPCCharacter* NPC);

	/**
	 * Demande qu’un PNJ “entre” dans un bâtiment.
	 * Si `bTeleportIfPossible` et si les points sont dans le même monde, on téléporte l’acteur.
	 * Sinon on détruit l’acteur (si présent) et on passe en simulation jusqu’au prochain chargement du monde cible.
	 */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence")
	void MoveNPCToBuilding(ATheHouseNPCCharacter* NPC, FName BuildingId, bool bInside, bool bTeleportIfPossible = true);

	/** Spawns quelques clients “fictifs” qui entrent dans un casino (cas #2) : état persistant + spawn si le monde est chargé. */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence")
	void SpawnSimulatedCasinoVisitors(FName BuildingId, TSubclassOf<ATheHouseNPCCharacter> NPCClass, int32 Count);

	/** Debug : récupère une copie des états. */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence")
	void GetAllNPCStates(TArray<FTheHouseNPCPersistentState>& OutStates) const;

	// =========================================================
	// SAVE / LOAD
	// =========================================================
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence|Save")
	bool SaveToSlot(const FString& SlotName = TEXT("TheHouseNPCPersistence"), int32 UserIndex = 0);

	UFUNCTION(BlueprintCallable, Category="NPC|Persistence|Save")
	bool LoadFromSlot(const FString& SlotName = TEXT("TheHouseNPCPersistence"), int32 UserIndex = 0);

	UFUNCTION(BlueprintPure, Category="NPC|Persistence|Save")
	int32 GetRandomSeed() const { return RandomSeed; }

	/** Remplace l’état runtime (utilisé par la sauvegarde globale). */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence|Save")
	void ApplyLoadedStates(const TArray<FTheHouseNPCPersistentState>& InStates, int32 InRandomSeed);

	/** Copie l’état courant depuis les acteurs vivants + états simulés (à appeler avant Save). */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence|Save")
	void SnapshotLiveNPCsIntoStates(UWorld* World);

	/** Après chargement : détruit les PNJ du monde courant puis respawn ceux qui appartiennent à ce niveau. */
	UFUNCTION(BlueprintCallable, Category="NPC|Persistence|Save")
	void RespawnNPCsForCurrentWorld(UWorld* World);

	/** Si un état existe pour cet id, retourne une copie (sinon false). */
	bool TryGetStateCopy(const FGuid& Id, FTheHouseNPCPersistentState& OutState) const;

private:
	UPROPERTY(Transient)
	TMap<FGuid, FTheHouseNPCPersistentState> States;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<ATheHouseNPCCharacter>, FGuid> LiveActorToId;

	void EnsureStateForActor(ATheHouseNPCCharacter* NPC);
	void CopyActorToState(ATheHouseNPCCharacter* NPC, FTheHouseNPCPersistentState& Out) const;
	bool ShouldExistInCurrentWorld(const UWorld* World, const FTheHouseNPCPersistentState& S) const;
	void SimulateState(FTheHouseNPCPersistentState& S, float NowSeconds);
	float GetNowSeconds() const;
	void TryRespawnIfRelevant(FTheHouseNPCPersistentState& S);

	/** Seed global (pour spawns déterministes). */
	UPROPERTY(Transient)
	int32 RandomSeed = 0;

	float LastLiveSnapshotSeconds = -1.f;
};

