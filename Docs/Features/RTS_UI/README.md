# RTS & menus UMG

Documentation rapide des **widgets RTS** et des **menus contextuels** (objets posés, PNJ, ordres). La référence détaillée (tables propriétés Blueprint, hit-test, double-clic) est dans le guide développeur.

| Sujet | Où lire |
|--------|---------|
| Panneau principal vs menu objet | [GUIDE_FR § Interface RTS](../../Developer/GUIDE_FR.md#interface-rts--panneau-principal-vs-menu-contextuel) · [GUIDE_EN § RTS UI](../../Developer/GUIDE_EN.md#rts-ui-main-panel-vs-context-menu) |
| Menus PNJ + ordres + exemple C++ | Même section (sous-parties *Menus PNJ* / *NPC menus*) |
| Classes C++ | `Source/TheHouse/UI/` — `TheHouseRTSMainWidget`, `TheHouseRTSContextMenuUMGWidget`, `TheHouseNPCRTSContextMenuUMGWidget`, `TheHouseNPCOrderContextMenuUMGWidget`, `TheHousePlacedObjectSettingsWidget`, `TheHouseRTSUIClickRelay`, `TheHouseFPSHudWidget` |
| Propriétés sur le PC | Souvent sur **`BP_HouseController`** : `RTS Main Widget Class`, `RTS Context Menu Widget Class`, `NPCRTSContextMenuWidgetClass`, `NPCOrderContextMenuWidgetClass`, `PlacedObjectSettingsWidgetClass` |

Retour : [Features](../README.md) · [Documentation TheHouse](../../README.md)
