# Features — documentation par sujet

Un dossier par **fonctionnalité** majeure. Chaque dossier contient un **`README.md`** autonome (reprise rapide, liens vers le guide long). Les **guides** `Docs/Developer/GUIDE_FR.md` et `GUIDE_EN.md` restent la référence la plus complète (entrées, tables, pièges).

## Index

| Dossier | Description |
|---------|-------------|
| [CasinoPlaceableObjects](CasinoPlaceableObjects/README.md) | `ATheHouseObject`, zones d’exclusion, grille, slots PNJ sur objet, placement, sélection |
| [SmartWall](SmartWall/README.md) | `ATheHouseSmartWall`, occlusion, `MinVisibleHeightCm`, matériau, Fill Cap, mesh pivot |
| [RTS_UI](RTS_UI/README.md) | Panneau RTS principal, menus contextuels (objet / PNJ / ordres), panneau paramètres, relais UI |
| [Localization](Localization/README.md) | Sous-système + SaveGame, pipeline CLI `RunLocalizationStep.ps1` |
| [NPCWorld](NPCWorld/README.md) | Subsystem, archetypes, volume d’éjection client, IA slot sur objet |

## Couverture code ↔ doc (résumé)

| Zone `Source/TheHouse/` | Doc feature / guide |
|-------------------------|---------------------|
| `Core/` (GameMode, HUD, Viewport, Selectable) | [GUIDE_FR](../../Developer/GUIDE_FR.md) sections GameMode, Viewport, HUD |
| `Player/` (PC, caméra RTS, FPS, murs) | Guide + [SmartWall](SmartWall/README.md) |
| `Object/` | [CasinoPlaceableObjects](CasinoPlaceableObjects/README.md) |
| `Placement/` | Guide section Placement |
| `AI/` | [NPCWorld](NPCWorld/README.md), [CasinoPlaceableObjects](CasinoPlaceableObjects/README.md) |
| `NPC/` | [NPCWorld](NPCWorld/README.md), guide § Système PNJ |
| `UI/` | [RTS_UI](RTS_UI/README.md), guide § Interface RTS |
| `Localization/` | [Localization](Localization/README.md), guide § Localisation |

Retour : [Documentation TheHouse](../README.md)
