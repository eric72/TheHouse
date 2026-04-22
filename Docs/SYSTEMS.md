# Systems — TheHouse

Résumé aligné sur le **code C++** actuel (`Source/TheHouse/`). Détail : `Docs/Developer/GUIDE_FR.md` / `GUIDE_EN.md` et `Docs/Features/`.

## RTS

- Déplacement plan, lacet, **zoom** (spring arm + `UTheHouseGameViewportClient` pour la molette en Game+UI).
- **Placement** : trace sol, `EPlacementState`, rotation preview (Shift + molette), grille `OccupiedGridCells`.
- **Sélection** : rectangle HUD, `ITheHouseSelectable`, double-clic pour panneau paramètres.
- **UI** : widget principal catalogue / argent / stock ; menus clic droit (objet, PNJ, ordres) — voir `Docs/Features/RTS_UI/`.

## Objets casino (`ATheHouseObject`)

- Empreinte **exclusion** vs autres objets ; états de placement ; sockets PNJ + tags.
- Économie : `PurchasePrice`, `SellValue`, options panneau (`bOpenParametersPanelOnLeftClick`, flux PNJ).

## PNJ

- **Personnage** : `ATheHouseNPCCharacter` + `ATheHouseNPCAIController` ; données **`UTheHouseNPCArchetype`** et sous-classes Staff / Customer / Special.
- **Registre** : `UTheHouseNPCSubsystem` (`GetNPCsByCategory`).
- **Volume** : `ATheHouseNPCEjectRegionVolume` pour ordres d’éjection depuis une zone.
- **Slots sur machines** : `UTheHouseObjectSlotUserComponent` (MoveTo, alignement, montage).

## Économie

- Argent joueur, catalogue, stock empilé par classe ; intégration UI RTS (vignettes optionnelles sur `FTheHousePlaceableCatalogEntry`).

## FPS

- `ATheHouseFPSCharacter` : exploration, sélectionnable en RTS si activé.
- Pas d’UI de gestion casino dédiée en mode FPS dans le périmètre actuel du C++.

## Murs & environnement

- **`ATheHouseSmartWall`** : alignement mesh, matériau, cap de remplissage — `Docs/Features/SmartWall/`.

## Localisation

- **Runtime** : `UTheHouseLocalizationSubsystem`, `UTheHouseLanguageSaveGame`.
- **Pipeline** : `Scripts/RunLocalizationStep.ps1` — `Docs/Features/Localization/`.
