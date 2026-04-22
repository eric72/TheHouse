// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Point d’entrée optionnel pour inclure tout le socle PNJ du projet TheHouse.
 *
 * Ordre conceptuel :
 * 1. TheHouseNPCTypes — énumérations.
 * 2. Archetypes/TheHouseNPCArchetype — racine ; Staff / Customer / Special — données par catégorie.
 * 3. TheHouseNPCIdentity — interface.
 * 4. TheHouseNPCCharacter — état runtime (Wallet, StaffMonthlySalary).
 * 5. TheHouseNPCAIController — IA.
 * 6. TheHouseNPCSubsystem — registre.
 */

#include "TheHouse/NPC/TheHouseNPCTypes.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCArchetype.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCStaffArchetype.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCCustomerArchetype.h"
#include "TheHouse/NPC/Archetypes/TheHouseNPCSpecialArchetype.h"
#include "TheHouse/NPC/TheHouseNPCIdentity.h"
#include "TheHouse/NPC/TheHouseNPCCharacter.h"
#include "TheHouse/NPC/TheHouseNPCAIController.h"
#include "TheHouse/NPC/TheHouseNPCSubsystem.h"
