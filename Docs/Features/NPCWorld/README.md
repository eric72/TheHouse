# PNJ & monde (hors PlayerController)

## Registre & catégories

`UTheHouseNPCSubsystem` expose les listes par **`ETheHouseNPCCategory`** (Staff, Customer, Special, …). Les données par rôle vivent dans des **Data Assets** sous `Source/TheHouse/NPC/Archetypes/` (racine **`UTheHouseNPCArchetype`** + Staff / Customer / Special).

## Volume d’éjection client

**`ATheHouseNPCEjectRegionVolume`** — acteur avec **`UBoxComponent`** : définit une zone d’où l’on peut ordonner l’**éjection** d’un client. Ne modifie pas la NavMesh. API : `ContainsWorldLocation`, `TryComputeEjectionExitWorldLocation` (sortie au-delà de la face dominante + `OutwardPushUU`).

## IA sur les objets casino

**`UTheHouseObjectSlotUserComponent`** — réservation de slot par tag, déplacement, alignement, montage (sans imposer un BT).

| Sujet | Où lire |
|--------|---------|
| Architecture PNJ complète | [GUIDE_FR § Système PNJ](../../Developer/GUIDE_FR.md#système-pnj-architecture) · [GUIDE_EN § NPC system](../../Developer/GUIDE_EN.md#npc-system-architecture) |
| Slots sur `ATheHouseObject` | [CasinoPlaceableObjects](../CasinoPlaceableObjects/README.md) |

Retour : [Features](../README.md) · [Documentation TheHouse](../../README.md)
