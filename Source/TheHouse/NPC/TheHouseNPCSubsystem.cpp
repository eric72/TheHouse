// Copyright Epic Games, Inc. All Rights Reserved.

#include "TheHouse/NPC/TheHouseNPCSubsystem.h"

#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouse/NPC/TheHouseNPCIdentity.h"

void UTheHouseNPCSubsystem::RegisterNPC(ATheHouseNPCCharacter* NPC)
{
	if (!IsValid(NPC))
	{
		return;
	}

	const ETheHouseNPCCategory Category = ITheHouseNPCIdentity::Execute_GetNPCCategory(NPC);
	FNPCWeakList& Bucket = NPCsByCategory.FindOrAdd(Category);

	const TWeakObjectPtr<ATheHouseNPCCharacter> WeakNPC(NPC);
	if (!Bucket.Contains(WeakNPC))
	{
		Bucket.Add(WeakNPC);
	}
}

void UTheHouseNPCSubsystem::UnregisterNPC(ATheHouseNPCCharacter* NPC)
{
	if (!IsValid(NPC))
	{
		return;
	}

	// Retrait dans toutes les listes : la catégorie peut avoir changé depuis Register (édition runtime).
	for (auto& Pair : NPCsByCategory)
	{
		Pair.Value.RemoveAll(
			[NPC](const TWeakObjectPtr<ATheHouseNPCCharacter>& Ptr) { return Ptr.Get() == NPC; });
	}
}

void UTheHouseNPCSubsystem::CompactBucket(const ETheHouseNPCCategory Category)
{
	if (FNPCWeakList* Bucket = NPCsByCategory.Find(Category))
	{
		Bucket->RemoveAll([](const TWeakObjectPtr<ATheHouseNPCCharacter>& Ptr) { return !Ptr.IsValid(); });
	}
}

TArray<ATheHouseNPCCharacter*> UTheHouseNPCSubsystem::GetNPCsByCategory(const ETheHouseNPCCategory Category) const
{
	// const_cast uniquement pour réutiliser CompactBucket (nettoyage des Weak expirés).
	UTheHouseNPCSubsystem* MutableThis = const_cast<UTheHouseNPCSubsystem*>(this);
	MutableThis->CompactBucket(Category);

	TArray<ATheHouseNPCCharacter*> Result;

	if (const FNPCWeakList* Bucket = NPCsByCategory.Find(Category))
	{
		Result.Reserve(Bucket->Num());
		for (const TWeakObjectPtr<ATheHouseNPCCharacter>& Weak : *Bucket)
		{
			if (ATheHouseNPCCharacter* NPC = Weak.Get())
			{
				Result.Add(NPC);
			}
		}
	}

	return Result;
}

int32 UTheHouseNPCSubsystem::GetNPCCountByCategory(const ETheHouseNPCCategory Category) const
{
	UTheHouseNPCSubsystem* MutableThis = const_cast<UTheHouseNPCSubsystem*>(this);
	MutableThis->CompactBucket(Category);

	if (const FNPCWeakList* Bucket = NPCsByCategory.Find(Category))
	{
		return Bucket->Num();
	}
	return 0;
}
