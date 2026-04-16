# Guide développeur — TheHouse

Documentation d’onboarding pour quiconque reprend le code C++ et les systèmes du projet.

**Moteur :** Unreal Engine **5.7** · **Module :** `TheHouse` (C++ runtime)

---

## Table des matières

1. [Vision du jeu](#vision-du-jeu)
2. [Architecture globale](#architecture-globale)
3. [Arborescence du code](#arborescence-du-code)
4. [Configuration & point d’entrée](#configuration--point-dentrée)
5. [Entrées : touches et souris](#entrées--touches-et-souris)
6. [GameMode](#gamemode-athehousegamemodebase)
7. [Viewport client (molette RTS)](#viewport-client-uthehousegameviewportclient)
8. [PlayerController (RTS / FPS)](#playercontroller-athehouseplayercontroller)
9. [Pion RTS : caméra](#pion-rts-athehousecamerapawn)
10. [Personnage FPS](#personnage-fps-athehousefpscharacter)
11. [HUD & sélection](#hud--sélection)
12. [Objets casino : `ATheHouseObject`](#objets-casino-athehouseobject)
13. [Placement (grille, prévisualisation)](#placement-grille-prévisualisation)
14. [Interface `ITheHouseSelectable`](#interface-ithehouseselectable)
15. [IA : utilisation des slots PNJ](#ia-utilisation-des-slots-pnj)
16. [Mur intelligent](#mur-intelligent)
17. [Debug & variables console](#debug--variables-console)
18. [Pièges fréquents](#pièges-fréquents)
19. [Références doc existantes](#références-doc-existantes)

---

## Vision du jeu

Résumé design (voir aussi `Docs/PROJECT_OVERVIEW.md` et `Docs/SYSTEMS.md`) :

- **Genre principal :** gestion de casino type tycoon / **RTS** (caméra aérienne, placement d’objets).
- **Activité secondaire :** **FPS** (exploration, missions) — le FPS **ne gère pas** la mécanique de gestion du casino.
- **Systèmes visés :** caméra RTS, placement sur grille, employés PNJ, économie, interaction avec les objets.

Le code C++ actuel couvre surtout **caméra RTS**, **bascule FPS**, **placement d’objets**, **sélection**, **slots PNJ** sur les objets, et des **helpers** (occlusion caméra, murs, etc.).

---

## Architecture globale

```
┌─────────────────────────────────────────────────────────────┐
│  ATheHouseGameModeBase                                       │
│  DefaultPawn = ATheHouseCameraPawn | PC = ATheHousePlayerController │
└──────────────────────────┬──────────────────────────────────┘
                           │
     ┌─────────────────────┼─────────────────────┐
     ▼                     ▼                     ▼
 UTheHouseGameViewportClient   ATheHouseHUD     (Blueprints carte)
 (molette → zoom RTS)          (rectangle sélection)
```

- **`ATheHousePlayerController`** : possède soit le **pion RTS** (`ATheHouseCameraPawn`), soit le **personnage FPS** (`ATheHouseFPSCharacter`). C’est le centre logique pour le zoom, le déplacement RTS, le placement, la sélection et le passage FPS.
- **`ATheHouseObject`** : acteur plaçable avec **zone d’exclusion** (empêche le chevauchement), états de placement, **slots PNJ** (sockets + tags).
- **`UTheHouseObjectSlotUserComponent`** : composant IA pour réserver un slot, se déplacer, s’aligner, jouer un montage — sans imposer un Behavior Tree.

---

## Arborescence du code

| Chemin | Rôle |
|--------|------|
| `Source/TheHouse/TheHouse.cpp` | Module principal (`IMPLEMENT_PRIMARY_GAME_MODULE`). |
| `Source/TheHouse/Core/TheHouseGameModeBase.*` | GameMode par défaut (pawn RTS, PC, HUD). |
| `Source/TheHouse/Core/TheHouseGameViewportClient.*` | Interception **MouseWheelAxis** pour le zoom. |
| `Source/TheHouse/Core/TheHouseHUD.*` | Dessin du rectangle de sélection RTS. |
| `Source/TheHouse/Core/TheHouseSelectable.h` | Interface **`ITheHouseSelectable`**. |
| `Source/TheHouse/Player/TheHousePlayerController.*` | **RTS/FPS**, placement, grille, sélection. |
| `Source/TheHouse/Player/TheHouseCameraPawn.*` | Spring arm, caméra RTS, **pitch sur le boom**, occlusion. |
| `Source/TheHouse/Player/TheHouseFPSCharacter.*` | Personnage FPS + sélectionnable. |
| `Source/TheHouse/Player/TheHouseSmartWall.*` | Acteur mur (détails : `Docs/Features/SmartWall/`). |
| `Source/TheHouse/Object/TheHouseObject.*` | Objet casino plaçable, footprint, PNJ. |
| `Source/TheHouse/Placement/TheHousePlacementTypes.h` | `EPlacementState` (None / Previewing / Placing). |
| `Source/TheHouse/AI/TheHouseObjectSlotUserComponent.*` | IA « premium » pour utiliser un slot sur un objet. |

**Blueprints :** le GameMode par défaut de la carte est souvent `BP_HouseGameMode` (`Config/DefaultEngine.ini`). Il **doit** hériter ou utiliser les classes C++ du projet pour que les correctifs d’entrée et le PC personnalisé s’appliquent.

---

## Configuration & point d’entrée

- **`TheHouse.uprojet`** : module `TheHouse`, plugin **Enhanced Input désactivé** — le projet s’appuie sur **anciennes Action/Axis mappings** (`DefaultInput.ini`) et sur des **contournements** (timer d’entrées, viewport) si le focus ou l’UI bloquent les touches.
- **`Config/DefaultEngine.ini`** :
  - `GameViewportClientClassName=/Script/TheHouse.TheHouseGameViewportClient` — nécessaire pour que la **molette** soit traitée avant les problèmes Game+UI.
  - `GlobalDefaultGameMode` : souvent un Blueprint ; vérifier que le **Player Controller Class** reste compatible avec `ATheHousePlayerController` (voir GameMode).

---

## Entrées : touches et souris

Les liaisons **nommées** (Action / Axis) sont définies dans **`Config/DefaultInput.ini`**. Le **`ATheHousePlayerController`** force un **`UInputComponent`** classique dans `SetupInputComponent()` (les lignes `DefaultPlayerInputClass=EnhancedPlayerInput` du `.ini` sont contournées côté code pour que `BindAction` / `BindAxis` fonctionnent).

### Actions

| Nom d’action | Touches (défaut `DefaultInput.ini`) | Liaison C++ | Rôle |
|--------------|-------------------------------------|-------------|------|
| `DebugToggleFPS` | **F** | `BindAction` → `DebugSwitchToFPS` | En RTS : passage FPS au sol sous le curseur. En FPS : retour RTS. |
| `RotateModifier` | **Alt gauche**, **clic droit** | Pressed / Released | Modificateur « rotation caméra » : avec ce modificateur actif, le clic gauche ne démarre pas la sélection ni ne confirme le placement. |
| `Select` | **Clic gauche** | Pressed / Released | RTS : début / fin du rectangle de sélection ; en **placement** (preview) : confirme le placement au clic (voir aussi repli LMB ci‑dessous). |
| `DebugPlaceObject` | **P** | Pressed | RTS : démarre le **placement** (preview) si `PendingPlacementClass` est défini ; si un preview est déjà actif, **annule** le placement. |

### Axes

| Nom d’axe | Entrées (défaut `DefaultInput.ini`) | Liaison C++ | Rôle |
|-----------|-------------------------------------|-------------|------|
| `MoveForward` | **Z**, **W** (+ avant) · **S** (− arrière) | `MoveForward` | Déplacement RTS sur le plan (pion caméra). |
| `MoveRight` | **D** (+ droite) · **Q**, **A** (− gauche) | `MoveRight` | Idem. |
| `Turn` | **MouseX** | `RotateCamera` | Lacet RTS (yaw du contrôleur). |
| `LookUp` | **MouseY** | `LookUp` | En FPS (personnage) : pitch caméra ; en RTS le pitch principal est géré sur le **spring arm** du `ATheHouseCameraPawn`. |
| `Zoom` | **MouseWheelAxis** | `Zoom` | Zoom RTS (longueur du spring arm). **Note :** une partie du zoom passe aussi par `UTheHouseGameViewportClient` → `TheHouse_ApplyWheelZoom` pour fiabiliser la molette en Game+UI. |

### Placement preview (RTS) : molette — zoom ou pivotement (Shift) ?

Conditions : **`EPlacementState::Previewing`**, pion possédé = **`RTSPawnReference`** (caméra RTS).

Le **preview**, c’est l’acteur **`ATheHouseObject`** instancié pour la prévisualisation (**`PreviewActor`** sur le `ATheHousePlayerController`) : le même type d’objet que celui qu’on placera, avant validation du placement.

| Situation | Effet |
|-----------|--------|
| **Molette sans Shift** | **Zoom / dézoom** caméra (spring arm) : même logique que hors placement — axe `Zoom`, `BindAxis`/`Zoom()`, et `TheHouse_ApplyWheelZoom` depuis le viewport. |
| **Shift (gauche ou droite) + molette** | **Pas de zoom** : `TheHouse_ApplyWheelZoom` **retourne immédiatement** (`IsInputKeyDown(LeftShift|RightShift)`). La rotation s’applique à ce **`ATheHouseObject`** de preview : dans **`PlayerTick`**, **`WasInputKeyJustPressed(MouseScrollUp)`** / **`MouseScrollDown`** incrémente ou décrémente le **yaw** de cet acteur par pas de **`PlacementRotationStepDegrees`** (défaut **15°**, éditable sur `ATheHousePlayerController`). |

**Précision :** ce n’est pas un « zoom sur le pivot » optique — c’est une **rotation** (orientation) de ce **fantôme `ATheHouseObject`** au sol autour de l’axe vertical (yaw).

### Autres comportements (uniquement dans le code)

| Comportement | Détail |
|--------------|--------|
| **Clic gauche (repli)** | Dans `PlayerTick`, si preview de placement actif : confirmation au **premier appui** détecté sur le bouton gauche (filet de secours si les Action Mappings posent problème). |
| **Timer d’entrées (~120 Hz)** | `TheHouse_PollInputFrame` interroge **physiquement** **Z, Q, S, D, W, A** via le viewport quand les axes classiques ne remontent pas (focus Slate, etc.). |

Pour modifier les touches : **Projet → Paramètres → Entrée** ou éditer `DefaultInput.ini`, en gardant les **noms** `MoveForward`, `Select`, etc., pour que le C++ reste valide.

---

## GameMode (`ATheHouseGameModeBase`)

Fichiers : `TheHouseGameModeBase.h/.cpp`

- **`DefaultPawnClass`** = `ATheHouseCameraPawn` (démarrage en RTS).
- **`PlayerControllerClass`** = `ATheHousePlayerController`.
- **`HUDClass`** = `ATheHouseHUD`.

**Important — `InitGame` :** si un GameMode Blueprint laisse le Player Controller sur la classe par défaut Epic, le code **force** `ATheHousePlayerController` et log une **erreur** explicite. Sans cela, ZQSD, zoom, placement et timers peuvent sembler « morts ».

---

## Viewport client (`UTheHouseGameViewportClient`)

Fichiers : `TheHouseGameViewportClient.h/.cpp`

- Surcharge **`InputAxis`** pour **`EKeys::MouseWheelAxis`**.
- En **Game+UI**, la molette n’atteint souvent pas `BindAxis` ; ici on appelle **`TheHouse_ApplyWheelZoom`** sur le `ATheHousePlayerController` et on **retourne `true`** sans appeler `Super` pour ce cas — évite un **double zoom**.

---

## PlayerController (`ATheHousePlayerController`)

Fichiers : `TheHousePlayerController.h/.cpp`

### Rôles principaux

1. **Référence au pion RTS** `RTSPawnReference` et au **FPS** `ActiveFPSCharacter`.
2. **Entrées RTS** : avant / droite sur le plan (axes), **zoom** (longueur du spring arm), **rotation** yaw (contrôleur) et pitch (via le **boom** du `ATheHouseCameraPawn`, pas le pitch du contrôleur seul).
3. **Timer `TheHouse_PollInputFrame` (~120 Hz)** : contournement quand `PlayerInput` / focus Slate / Blueprint **Event Tick** sans `Parent` cassent les axes. Polling **physique** des touches (ZQSD) via l’état du viewport.
4. **Placement** : `DebugStartPlacement`, `UpdatePlacementPreview`, `ConfirmPlacement`, `CancelPlacement`, état `EPlacementState`, acteur **`PreviewActor`**.
5. **Grille** : `FTheHousePlacementGridSettings` (`CellSize`, `WorldOrigin`, `MinUpNormalZ`, `bEnableGridSnap`), cache **`OccupiedGridCells`**.
6. **Sélection** : clic maintenu + HUD ; au relâchement, actors dans le rectangle implémentant **`ITheHouseSelectable`** reçoivent **`OnSelect`**.

### Bascule RTS ↔ FPS

- **`SwitchToRTS()`** : annule le placement, synchronise la position de la caméra RTS avec le FPS (si hauteur raisonnable), repossède le pion RTS, **détruit** le personnage FPS, curseur visible, `FInputModeGameAndUI`.
- **`SwitchToFPS(TargetLocation, TargetRotation)`** : spawn `FPSCharacterClass` (défaut : classe C++), repossession, masque certains **meshes de test** (cube), `FInputModeGameOnly`, timer pour rétablir la gravité/chute.
- **`DebugSwitchToFPS()`** : depuis la vue RTS, **trace** depuis la souris (deproject + line trace) pour trouver le **sol** ; sinon repli sur la position du pivot RTS.

### Zoom molette

- Paramètres : `RTSWheelZoomMultiplier`, `RTSZoomSpeed`, `RTSMinArmDeltaPerWheelEvent` (évite zooms infimes sur souris haute précision), `bRTSZoomScalesWithArmLength`, `RTSCursorZoomMaxPanPerEvent`, `RTSZoomInterpSpeed`, etc.
- Pendant le **placement** : **Shift + molette** = **rotation** du preview (pas de zoom) ; géré dans **`PlayerTick`**.

### Structure placement (aperçu)

- Trace sous le curseur → surface « sol » (`ValidatePlacement` : normale Z ≥ `MinUpNormalZ`).
- Snap XY optionnel, rotation yaw = `PlacementPreviewYawDegrees`.
- **`ATheHouseObject::EvaluatePlacementAt`** : chevauchement autres objets + monde.
- Grille : empreinte approximative de la **boîte d’exclusion** projetée en cellules ; collision avec `OccupiedGridCells`.

---

## Pion RTS (`ATheHouseCameraPawn`)

Fichiers : `TheHouseCameraPawn.h/.cpp`

- **`USpringArmComponent`** + **`UCameraComponent`** + **`UFloatingPawnMovement`**.
- **Pitch RTS** : stocké dans **`RTSArmPitchDegrees`** et appliqué au boom (**`SyncRTSArmPitchToBoom`**), pas seulement via le Control Rotation — évite les conflits avec le yaw.
- **Occlusion** : `CheckCameraOcclusion` — fade de matériau sur les acteurs entre caméra et pawn (paramètre type `CameraFade`), sweep, gestion anti-scintillement.

---

## Personnage FPS (`ATheHouseFPSCharacter`)

Fichiers : `TheHouseFPSCharacter.h/.cpp`

- Hérite de **`ACharacter`**, implémente **`ITheHouseSelectable`**.
- **`bCanBeSelected`** : peut désactiver la sélection depuis le contrôleur RTS.
- Entrées mouvement en booléens (pressed/released) sur les mappings du projet.

---

## HUD & sélection

- **`ATheHouseHUD`** : `StartSelection` / `UpdateSelection` / `StopSelection` / `DrawHUD` pour le rectangle.
- La logique « quels acteurs sont dans le rectangle » est dans le **PlayerController** (réutilisation des positions à l’écran / sélection).

---

## Objets casino (`ATheHouseObject`)

Fichiers : `TheHouseObject.h/.cpp`

### Composants

- **`ObjectMesh`** : mesh principal (collisions, navigation).
- **`ExclusionZoneVisualizer`** : volume de prévisualisation (souvent cube moteur mis à l’échelle), **sans collision** pour la prévisualisation.

### Zone d’exclusion

- **`PlacementExclusionHalfExtent`**, **`PlacementExclusionCenterOffset`** : boîte locale ; **`GetWorldPlacementExclusionBox`** pour tests.
- **`TestFootprintOverlapsOthersAt`** : intersection de boîtes avec les autres `ATheHouseObject`.
- **`TestFootprintOverlapsWorldAt`** : blocage par le monde (hors acteur ignoré, typiquement le sol sous le curseur).

### États (`EObjectPlacementState`)

`Valid`, `OverlapsObject`, `OverlapsWorld`, `OverlapsGrid` — pilotent matériaux valide / invalide.

### PNJ

- **`FTheHouseNPCInteractionSlot`** : `SocketName`, `PurposeTag`.
- **`TryReserveNPCSlot`**, **`ReleaseNPCSlotByIndex`**, **`GetNPCSlotWorldTransform`**, etc. Les occupants sont suivis dans **`NPCSlotOccupants`**.

### Sélection (`ITheHouseSelectable`)

- **`OnSelect` / `OnDeselect`** : surbrillance (**Custom Depth** / stencil), etc.

---

## Placement (grille, prévisualisation)

- Types : `Source/TheHouse/Placement/TheHousePlacementTypes.h` — **`EPlacementState`** pour le flux **PlayerController** (None, Previewing, Placing).
- **`ConfirmPlacement`** : vérifie validité, **`MarkCellsOccupied`**, **`FinalizePlacement`** sur l’objet, réinitialise l’état.
- Debug console : **`TheHouse.Placement.Debug`** (voir section Debug).

---

## Interface `ITheHouseSelectable`

Fichier : `TheHouseSelectable.h`

- **`OnSelect`**, **`OnDeselect`**, **`IsSelectable`** (défaut vrai).
- Utilisé par **`ATheHouseObject`**, **`ATheHouseFPSCharacter`**, et tout acteur Blueprint qui implémente l’interface en C++ ou via Blueprint (selon usage).

---

## IA : utilisation des slots PNJ

Fichiers : `TheHouseObjectSlotUserComponent.h/.cpp`

- États : **`EObjectSlotUseState`** (Idle, MovingToSlot, Aligned, PlayingMontage).
- **`UseObjectSlot(TargetObject, PurposeTag, Montage)`** : réserve un slot, **`MoveTo`** via AIController, alignement interpolé, montage optionnel.
- Paramètres : `AcceptableRadius`, `AlignInterpSpeed`.

---

## Mur intelligent

L’acteur **`ATheHouseSmartWall`** et la doc détaillée : **`Docs/Features/SmartWall/README.md`** (Fill Cap, matériaux, etc.).

---

## Debug & variables console

| Variable | Effet |
|----------|--------|
| `TheHouse.Placement.Debug` | Logs du flux de placement (0 = off, 1 = on). |
| `TheHouse.FPS.Debug` | Logs trace / meshes devant la caméra FPS (0/1). |

Le PlayerController log aussi des messages utiles au **BeginPlay** (classe `PlayerInput`, avertissement Enhanced Input).

---

## Pièges fréquents

1. **GameMode Blueprint** sans `ATheHousePlayerController` → `InitGame` corrige mais il faut aligner le BP pour éviter la confusion.
2. **`DefaultInput.ini` repassé en Enhanced Input** alors que le projet désactive le plugin → entrées incohérentes ; les logs au démarrage le signalent.
3. **Blueprint PlayerController / Pawn** avec **Event Tick** sans appeler **Parent** → le tick C++ du contrôleur peut ne pas tourner correctement.
4. **Double zoom** : ne pas réintroduire un second chemin molette en parallèle du `GameViewportClient`.
5. **Placement** : surfaces trop pentues rejetées (`MinUpNormalZ`) ; pas de hit sous curseur = invalide.

---

## Références doc existantes

| Document | Contenu |
|----------|---------|
| `Docs/README.md` | Index de la documentation Features / Changelog. |
| `Docs/PROJECT_OVERVIEW.md` | Vision produit courte. |
| `Docs/SYSTEMS.md` | Liste des systèmes jeu. |
| `Docs/Features/CasinoPlaceableObjects/README.md` | Objets plaçables, workflow BP. |
| `Docs/Features/SmartWall/README.md` | Smart wall. |
| `Docs/Changelog/CHANGELOG.md` | Journal des versions. |

---

## Site web (documentation)

Une version **HTML** (navigation + thème) est disponible dans **`Docs/site/`** : ouvrir `index.html` dans un navigateur (idéalement via un serveur statique local si les navigateurs bloquent les chemins relatifs stricts).

*Dernière mise à jour : alignée sur le code du module `TheHouse` (UE 5.7).*
