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
- **Recrutement staff (roster)** : configuré sur `ATheHousePlayerController` via `NPCStaffRecruitmentPool` (règles) → génère `NPCStaffRosterOffers` (liste affichée, avec vignettes, noms, étoiles, salaires).
- **Temps in‑game + paie** : horloge compressée (durée d’une journée en secondes IRL) + **payroll quotidien** (débit des salaires des employés après ≥ 24h in‑game).

## FPS

- **Bascule RTS ↔ FPS** : `ATheHousePlayerController::SwitchToFPS` / `SwitchToRTS`.
- **Contrôles FPS** (remappables via `DefaultInput.ini` / écran options) : saut, course, accroupi, tir.
- **Caméra FPS** : attachable à un socket (ex. `HeadCameraEmplacement`) ou à la capsule ; free-look optionnel (corps qui suit après un seuil) + réalignement en mouvement.
- **Animation (locomotion)** : `UTheHouseLocomotionAnimInstance` expose Speed/Direction + états air/landing + offsets spine pour AimOffset (joueur & PNJ).
- **PV & mort** : `UTheHouseHealthComponent` (ApplyDamage) ; ragdoll auto (désactivation capsule + simulate physics).
- **Combat** : `ATheHouseWeapon` + `ATheHouseProjectile` (spawn depuis le muzzle socket `Muzzle`, fallback hitscan).

## Murs & environnement

- **`ATheHouseSmartWall`** : alignement mesh, matériau, cap de remplissage — `Docs/Features/SmartWall/`.

## Localisation

- **Runtime** : `UTheHouseLocalizationSubsystem`, `UTheHouseLanguageSaveGame`.
- **Pipeline** : `Scripts/RunLocalizationStep.ps1` — `Docs/Features/Localization/`.
