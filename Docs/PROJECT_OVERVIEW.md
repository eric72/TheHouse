# Game overview — TheHouse

**Working title ideas:** *The House* (direction actuelle), *Sum Zero*, *Zero Sum*.

## Genre

- Gestion de casino type tycoon / **RTS** (caméra aérienne, placement, économie).
- **FPS** en activité secondaire (exploration) — le FPS **ne pilote pas** la gestion casino.

## Règle centrale

- Le **RTS** porte la boucle principale (placement, sélection, UI catalogue / stock, PNJ).
- Le **FPS** n’expose pas les systèmes de gestion du casino.

## Systèmes principaux (implémentation actuelle)

- **Caméra RTS** : pion ressort, zoom (viewport + axe), pitch sur boom, occlusion.
- **Placement** : grille, prévisualisation `ATheHouseObject`, validation sol / chevauchement.
- **Objets casino** : `ATheHouseObject`, zone d’exclusion, slots PNJ, panneau paramètres UMG.
- **UI RTS** : panneau principal, menus contextuels (objet, PNJ, ordres), relais clics catalogue.
- **PNJ** : `ATheHouseNPCCharacter`, archetypes Data Asset, subsystem par catégorie, volume d’éjection client, IA slot sur objet.
- **Économie** : prix catalogue, vente, stock (côté PlayerController / widgets).
- **Murs** : `ATheHouseSmartWall` (occlusion, Fill Cap expérimental).
- **Localisation** : sous-système + SaveGame ; pipeline CLI sous `Scripts/`.

## Documentation

- Vue technique détaillée : **`Docs/Developer/GUIDE_FR.md`** / **`GUIDE_EN.md`**.
- Features par dossier : **`Docs/Features/`** (voir **`Docs/Features/README.md`** pour l’index).
- Site statique : **`Docs/site/`**.

---

## English (short)

Casino **RTS** management + secondary **FPS**; FPS does not run casino systems. Main C++ areas: RTS camera & UI, placement, `ATheHouseObject`, NPC foundation (archetypes, subsystem, eject volume), localization, SmartWall. Docs: **`Docs/Developer/GUIDE_EN.md`**, **`Docs/Features/`**, **`Docs/site/`**.
