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
9. [Interface RTS : panneau principal vs menu contextuel](#interface-rts--panneau-principal-vs-menu-contextuel)
10. [Pion RTS : caméra](#pion-rts-athehousecamerapawn)
11. [Personnage FPS](#personnage-fps-athehousefpscharacter)
12. [HUD & sélection](#hud--sélection)
13. [Objets casino : `ATheHouseObject`](#objets-casino-athehouseobject)
14. [Placement (grille, prévisualisation)](#placement-grille-prévisualisation)
15. [Interface `ITheHouseSelectable`](#interface-ithehouseselectable)
16. [IA : utilisation des slots PNJ](#ia-utilisation-des-slots-pnj)
17. [Système PNJ (architecture)](#système-pnj-architecture)
18. [Mur intelligent](#mur-intelligent)
19. [Debug & variables console](#debug--variables-console)
20. [Pièges fréquents](#pièges-fréquents)
21. [Références doc existantes](#références-doc-existantes)
22. [Localisation (pipeline complet en ligne de commande)](#localisation-pipeline-complet-en-ligne-de-commande)

---

## Vision du jeu

Résumé design (voir aussi `Docs/PROJECT_OVERVIEW.md` et `Docs/SYSTEMS.md`) :

- **Genre principal :** gestion de casino type tycoon / **RTS** (caméra aérienne, placement d’objets).
- **Activité secondaire :** **FPS** (exploration, missions) — le FPS **ne gère pas** la mécanique de gestion du casino.
- **Systèmes visés :** caméra RTS, placement sur grille, employés PNJ, économie, interaction avec les objets, **UI RTS (UMG)**, **localisation** (texte + pipeline).

Le code C++ actuel couvre surtout **caméra RTS**, **bascule FPS**, **placement d’objets**, **sélection**, **UI RTS** (panneau principal, menus contextuels objets / PNJ / ordres, panneau paramètres), **slots PNJ** sur les objets, un **socle PNJ** (catégories, archetypes Data Asset, registre monde, volume d’éjection client), **localisation** (sous-système + SaveGame, pipeline CLI), et des **helpers** (occlusion caméra, murs, etc.).

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

- **`ATheHousePlayerController`** : possède soit le **pion RTS** (`ATheHouseCameraPawn`), soit le **personnage FPS** (`ATheHouseFPSCharacter`). C’est le centre logique pour le zoom, le déplacement RTS, le placement, la sélection et le passage FPS. Il **ne pilote pas** les PNJ du casino (voir **système PNJ** ci‑dessous).
- **`ATheHouseObject`** : acteur plaçable avec **zone d’exclusion** (empêche le chevauchement), états de placement, **slots PNJ** (sockets + tags).
- **`UTheHouseObjectSlotUserComponent`** : composant IA pour réserver un slot, se déplacer, s’aligner, jouer un montage — sans imposer un Behavior Tree.
- **`ATheHouseNPCCharacter` + `ATheHouseNPCAIController` + `UTheHouseNPCSubsystem`** : personnages PNJ possédés par un **AIController**, enregistrés par **catégorie** ; configuration par **Data Assets** dans `NPC/Archetypes/` (**Staff / Customer / Special**, tous dérivés de **`UTheHouseNPCArchetype`** — classe racine non abstraite pour le sélecteur d’assets de l’éditeur).

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
| `Source/TheHouse/NPC/TheHouseNPCTypes.h` | `ETheHouseNPCCategory`, `ETheHouseNPCSpecialKind` (police, pompier, médecin, …). |
| `Source/TheHouse/NPC/Archetypes/TheHouseNPCArchetype.*` | Base **`UTheHouseNPCArchetype`** + **`…Staff` / `…Customer` / `…Special`** — données séparées par catégorie. |
| `Source/TheHouse/NPC/TheHouseNPCIdentity.h` | Interface **`ITheHouseNPCIdentity`** (`GetNPCCategory`, `GetNPCArchetype`). |
| `Source/TheHouse/NPC/TheHouseNPCCharacter.*` | **`ATheHouseNPCCharacter`** — état runtime (`Wallet`), `ApplyArchetypeDefaults`, possession IA. |
| `Source/TheHouse/NPC/TheHouseNPCAIController.*` | **`ATheHouseNPCAIController`** — point d’entrée BT / perception (à étendre). |
| `Source/TheHouse/NPC/TheHouseNPCSubsystem.*` | **`UTheHouseNPCSubsystem`** — registre par catégorie dans le monde (`GetNPCsByCategory`). |
| `Source/TheHouse/NPC/TheHouseNPC.h` | Include unique optionnel de tout le socle PNJ. |
| `Source/TheHouse/NPC/TheHouseNPCEjectRegionVolume.*` | Volume boîte : zone d’où l’on peut ordonner l’**éjection** d’un client ; sortie le long de la face dominante (`TryComputeEjectionExitWorldLocation`). |
| `Source/TheHouse/Localization/TheHouseLocalizationSubsystem.*` | Langue active, application aux widgets texte. |
| `Source/TheHouse/Localization/TheHouseLanguageSaveGame.*` | Persistance du choix de langue (`USaveGame`). |
| `Source/TheHouse/UI/TheHouseRTSMainWidget.*` | Panneau RTS principal (argent, catalogue, stock) — `BindWidgetOptional` sur `MoneyText`, `CatalogScroll`, `StoredScroll`. |
| `Source/TheHouse/UI/TheHouseRTSContextMenuUMGWidget.*` / `TheHouseRTSContextMenuWidget.*` | Menu contextuel **objet** posé (Slate ou UMG). |
| `Source/TheHouse/UI/TheHouseNPCRTSContextMenuUMGWidget.*` | Menu contextuel **PNJ** (sans sélection préalable). |
| `Source/TheHouse/UI/TheHouseNPCOrderContextMenuUMGWidget.*` | Menu **ordres PNJ** (sol / objet / autre PNJ) quand des PNJ sont sélectionnés. |
| `Source/TheHouse/UI/TheHousePlacedObjectSettingsWidget.*` | Panneau paramètres d’un `ATheHouseObject` posé. |
| `Source/TheHouse/UI/TheHouseFPSHudWidget.*` | HUD UMG côté FPS (si branché). |
| `Source/TheHouse/UI/TheHouseRTSUIClickRelay.*` | Relais de clics catalogue / stock vers le PlayerController. |
| `Source/TheHouse/Player/BP_SmartWall.*` | Classe **Blueprint natif** liée au mur intelligent (voir `Docs/Features/SmartWall/`). |

**Blueprints :** le GameMode par défaut de la carte est souvent `BP_HouseGameMode` (`Config/DefaultEngine.ini`). Il **doit** hériter ou utiliser les classes C++ du projet pour que les correctifs d’entrée et le PC personnalisé s’appliquent.

---

## Configuration & point d’entrée

- **`TheHouse.uproject`** : module `TheHouse`, plugin **Enhanced Input désactivé** — le projet s’appuie sur **anciennes Action/Axis mappings** (`DefaultInput.ini`) et sur des **contournements** (timer d’entrées, viewport) si le focus ou l’UI bloquent les touches.
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

### Sélection RTS : cadre, UMG, panneau paramètres

- **`Input_SelectPressed` / `Input_SelectReleased`** (`TheHousePlayerController.cpp`) : en mode RTS (pion = caméra), le **clic gauche** démarre un état **`bIsSelecting`** ; le timer **`TheHouse_PollInputFrame`** appelle **`ATheHouseHUD::UpdateSelection`** pour le rectangle vert.
- **`Input_SelectDoubleClick`** : le PlayerController bind aussi **`EKeys::LeftMouseButton` + `IE_DoubleClick`**. Ce chemin sert de repli quand le **2e clic** est absorbé par l’UMG : un double clic sur un objet peut quand même refaire une trace monde et rouvrir le panneau de paramètres.
- **`bRtsSelectGestureFromWorld`** : mis à **true** seulement si le press a réellement démarré ce drag depuis le « monde » (pas un early-return). Au **release**, si ce flag est **false**, toute la logique rectangle / clic unique est ignorée (évite d’utiliser un **`SelectionStartPos`** obsolète après un clic sur l’UI).
- **`TheHouse_IsCursorOverBlockingRTSViewportUI`** : décide si le curseur est sur de l’UMG qui **doit** prendre le clic à la place du monde. Le **menu contextuel** et le **WBP de paramètres d’objet** utilisent la **géométrie** du widget (`IsUnderLocation`). Le **panneau RTS principal** n’utilise **plus** seule la géométrie du root (souvent plein écran) : on inspecte le **chemin Slate** sous la souris et un UMG en **`Visible`** (hit-testable) dans l’arbre du WBP ; les zones **`SelfHitTestInvisible`** / **`HitTestInvisible`** laissent passer le clic vers le monde (rectangle de sélection, clic sur objet).
- **Panneau paramètres** : propriété **`PlacedObjectSettingsWidgetClass`** sur le Blueprint Player Controller (souvent **`BP_HouseController`**) ; sur l’objet **`bOpenParametersPanelOnLeftClick`** (+ champs économie **`bNpcMoneyFlowEnabled`**, **`NpcSpendToPlay`**, **`NpcMaxReturnToHouse`**). Ouverture via **`OpenPlacedObjectSettingsWidgetFor`** après sélection ; fermeture **`ClosePlacedObjectSettingsPanel`** (Blueprint), Échap (**`TryConsumeEscapeForRtsUi`**), ou vente depuis le menu. Un **second clic** sur le **même** objet rafraîchit le WBP existant (**`SetTargetPlacedObject`**) sans le recréer.
- **Checklist Blueprint à ne pas oublier** : dans **`BP_HouseController`**, renseigner **`PlacedObjectSettingsWidgetClass`**, **`RTS Main Widget Class`**, **`RTS Context Menu Widget Class`** si tu utilises des WBP dédiés. Dans chaque BP d’objet casino, vérifier **`bOpenParametersPanelOnLeftClick`**, **`PurchasePrice`**, **`SellValue`**.

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

## Interface RTS : panneau principal vs menu contextuel

Le `ATheHousePlayerController` référence **deux** types de widget différents (ce n’est pas un doublon) :

| Propriété (souvent dans **Class Defaults** du Blueprint Player Controller, ex. `BP_HouseController`) | Classe C++ de référence | Rôle |
|---------------------------------------------------------------------------------------------------|-------------------------|------|
| **`RTS Main Widget Class`** | `UTheHouseRTSMainWidget` | **Panneau RTS persistant** : argent, catalogue de pose, stock (quantités par type). Affiché tant que le joueur est en **mode RTS**. |
| **`RTS Context Menu Widget Class`** | `UTheHouseRTSContextMenuWidget` (Slate C++) ou `UTheHouseRTSContextMenuUMGWidget` / WBP enfant | **Menu contextuel** au **clic droit** sur un **`ATheHouseObject`** déjà posé (ex. *Vendre*, *Mettre en stock*). Fenêtre temporaire sous la souris, fermée après l’action. |

### Menus PNJ et ordres (clic gauche = sélection, clic droit = menu/ordre)

Trois menus distincts existent en RTS :

| Cas | Classe UI (parent WBP) | Propriété à renseigner sur le BP PlayerController |
|-----|-------------------------|---------------------------------------------------|
| **Menu PNJ “inspect/use…”** (clic droit sur un PNJ **sans** PNJ sélectionné) | `UTheHouseNPCRTSContextMenuUMGWidget` | `NPCRTSContextMenuWidgetClass` |
| **Menu ordres PNJ (cible objet / PNJ / sol)** (clic droit avec **au moins un PNJ sélectionné au clic gauche**) | `UTheHouseNPCOrderContextMenuUMGWidget` | `NPCOrderContextMenuWidgetClass` |
| **Menu objet placé** (clic droit sur un `ATheHouseObject` sans PNJ sélectionné) | `UTheHouseRTSContextMenuUMGWidget` | `RTSContextMenuWidgetClass` |

Variables UMG “bindées” (optionnelles, `BindWidgetOptional`) dans les WBP de menus :
- `MenuBorder` (`UBorder`)
- `OptionsBox` (`UVerticalBox`)

### Ajouter des actions d’ordres (exemple C++)

Les actions sont **dynamiques** : tu les ajoutes à l’ouverture du menu via les events BlueprintNativeEvent.

- `GatherNPCOrderActionsOnGround(OutOptionDefs)` : ordres sur le sol (point cliqué).
- `ExecuteNPCOrderOnGroundAction(OptionId)` : exécute l’option sur les PNJ sélectionnés. Le point est accessible via `GetLastNPCOrderGroundTarget()`.

Exemple (dans une sous-classe C++ de `ATheHousePlayerController`) :

```cpp
namespace TheHouseNPCOrderIds
{
	static const FName MoveTo(TEXT("MoveTo"));
	static const FName RunTo(TEXT("RunTo"));
	static const FName DebugShout(TEXT("DebugShout"));
}

void AMyHousePlayerController::GatherNPCOrderActionsOnGround_Implementation(
	TArray<FTheHouseRTSContextMenuOptionDef>& OutOptionDefs)
{
	using namespace TheHouseNPCOrderIds;

	OutOptionDefs.Reset();
	OutOptionDefs.Add({ MoveTo, FText::FromString(TEXT("Se déplacer ici")) });
	OutOptionDefs.Add({ RunTo,  FText::FromString(TEXT("Courir ici")) });
	OutOptionDefs.Add({ DebugShout, FText::FromString(TEXT("Crier (debug)")) });
}

void AMyHousePlayerController::ExecuteNPCOrderOnGroundAction_Implementation(FName OptionId)
{
	using namespace TheHouseNPCOrderIds;

	TArray<ATheHouseNPCCharacter*> NPCs;
	GetSelectedNPCsForRTSOrders(NPCs);

	const FVector Goal = GetLastNPCOrderGroundTarget();

	if (OptionId == MoveTo || OptionId == RunTo)
	{
		for (ATheHouseNPCCharacter* Npc : NPCs)
		{
			if (!IsValid(Npc)) continue;
			if (AAIController* AI = Cast<AAIController>(Npc->GetController()))
			{
				FAIMoveRequest Req;
				Req.SetGoalLocation(Goal);
				Req.SetAcceptanceRadius(52.f);
				Req.SetUsePathfinding(true);
				Req.SetAllowPartialPath(true);
				AI->MoveTo(Req);
			}
		}
	}
	else if (OptionId == DebugShout)
	{
		for (ATheHouseNPCCharacter* Npc : NPCs)
		{
			if (IsValid(Npc))
			{
				UE_LOG(LogTemp, Warning, TEXT(\"[Order] %s shout!\"), *Npc->GetName());
			}
		}
	}
}
```

**Personnalisation du panneau principal** : Widget Blueprint dont le parent est **`TheHouse RTS Main Widget`**, avec trois widgets nommés exactement **`MoneyText`**, **`CatalogScroll`**, **`StoredScroll`** (`BindWidgetOptional` côté C++) ; assigner ce WBP dans **`RTS Main Widget Class`**. Détails et pièges : commentaires dans `Source/TheHouse/UI/TheHouseRTSMainWidget.h`.

**Vignettes (thumbnails)** : dans **`PlaceableCatalog`** (Class Defaults du Player Controller), chaque entrée peut définir **`Thumbnail`** (`Texture 2D`) ; le panneau RTS affiche l’image à côté du nom dans le **catalogue** et dans le **stock** lorsque la même **`ObjectClass`** existe au catalogue.

**Listes visibles seulement au clic (ex. barre d’icônes)** : le C++ remplit toujours `CatalogScroll` / `StoredScroll` ; pour les **masquer par défaut**, mets-les dans un **`Border`** / **`WidgetSwitcher`** / panneau dont tu changes **`Visibility`** (Collapsed / Visible) depuis le **graph** du WBP (bouton icône → `SetVisibility`), idéalement après l’événement **After RTS Main Refreshed** si tu dois resynchroniser l’état.

**Valeur `None` sur `RTS Main Widget Class`** : au runtime, si la propriété reste vide, `InitializeModeWidgets()` affecte la classe C++ par défaut `UTheHouseRTSMainWidget`.

**Hit-test et sélection** : un **Canvas / Border** racine en **`Visible`** qui couvre tout l’écran fait encore passer le curseur pour « sur l’UI » (blocage monde). Pour une zone décorative plein écran, préférer **`SelfHitTestInvisible`** (ou **`HitTestInvisible`**) sur le conteneur ; réserver **`Visible`** aux boutons, listes, textes interactifs.

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
- Détail du filtre UMG + **`bRtsSelectGestureFromWorld`** : voir **PlayerController → Sélection RTS** ci-dessus.

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

### Panneau paramètres (casino)

- **`bOpenParametersPanelOnLeftClick`** : si coché sur le BP de l’objet, un **clic gauche RTS** qui sélectionne cet objet ouvre le WBP (**`PlacedObjectSettingsWidgetClass`** sur le Player Controller).
- **Économie PNJ (affichage WBP)** : **`bNpcMoneyFlowEnabled`**, **`NpcSpendToPlay`**, **`NpcMaxReturnToHouse`** — lus côté UI dans l’événement **After Target Placed Object Changed** (voir `TheHousePlacedObjectSettingsWidget.h`).
- **Revente** : **`SellValue`** est la valeur prioritaire quand on utilise **Vendre**. Si **`SellValue == 0`**, le code prend maintenant **`PurchasePrice`** comme repli. En pratique : renseigner **`SellValue`** si la revente doit être différente du prix d’achat ; sinon laisser **0** pour « reprendre le prix d’achat ».

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

## Système PNJ (architecture)

### Principe : pas dans le PlayerController

Les **PNJ** ne sont **pas** des joueurs humains : ils ne doivent **pas** être pilotés par `ATheHousePlayerController`. En runtime Unreal, un PNJ est typiquement un **`ACharacter`** possédé par un **`AAIController`** — ici **`ATheHouseNPCAIController`**. Le PlayerController reste réservé au joueur (RTS / FPS).

### Catégories métier (`ETheHouseNPCCategory`)

| Valeur | Usage |
|--------|--------|
| **`Unassigned`** | Archetype absent ou invalide — à corriger en édition. |
| **`Staff`** | Personnel : salaire par défaut (`DefaultMonthlySalary` sur l’archetype Staff) → copié dans **`StaffMonthlySalary`** sur le personnage (**modifiable en UI**). |
| **`Customer`** | Client : **`StartingWallet`** → **`Wallet`** ; **`OptionalAdmissionFee`** uniquement sur l’archetype client (0 = désactivé, usage futur). |
| **`Special`** | PNJ « service » : **`ETheHouseNPCSpecialKind`** + **`SpecialDutyId`** sur l’archetype spécial. |

La catégorie sert au **registre** (`UTheHouseNPCSubsystem`), à l’UI et aux règles futures. Une **sous-classe C++ par catégorie** évite les champs hors-sujet (pas de prix d’entrée sur un asset Staff ou Police).

### Données : `Source/TheHouse/NPC/Archetypes/`

- **`UTheHouseNPCArchetype`** (racine, à éviter seule en jeu) — **`DisplayName`**, **`Category`** (rempli par les sous-classes). Préférez toujours Staff / Customer / Special pour les `.uasset`.
- **`UTheHouseNPCStaffArchetype`** — **`StaffRoleId`**, **`DefaultMonthlySalary`**, présentation UI (portrait, couleur).
- **`UTheHouseNPCCustomerArchetype`** — **`StartingWallet`**, **`OptionalAdmissionFee`** (clients uniquement).
- **`UTheHouseNPCSpecialArchetype`** — **`SpecialKind`**, **`SpecialDutyId`**.

Créez les `.uasset` avec le type correspondant (**TheHouse NPC Staff Archetype**, etc.). Le **`NPCArchetype`** sur le personnage reste typé **`UTheHouseNPCArchetype*`** (polymorphisme).

### Identité : `ITheHouseNPCIdentity`

Permet au gameplay et à l’UI d’interroger un acteur sans connaître **`ATheHouseNPCCharacter`** en dur :

- **`GetNPCCategory`**
- **`GetNPCArchetype`**

### Personnage : `ATheHouseNPCCharacter`

- **`NPCArchetype`** — référence au Data Asset (modifiable sur le BP ou l’instance de niveau).
- **`Wallet`** — clients ; **`TrySpendFromWallet`** / **`AddToWallet`**.
- **`StaffMonthlySalary`** — personnel ; initialisé depuis **`DefaultMonthlySalary`** ; **`SetStaffMonthlySalary`** pour l’UI joueur.
- **`ApplyArchetypeDefaults`** — *BlueprintNativeEvent* : remplit **`Wallet`** ou **`StaffMonthlySalary`** selon le Cast ; rappeler le parent si vous surchargez en Blueprint.
- Au **`BeginPlay`** : application des défauts, puis **`RegisterNPC`** sur **`UTheHouseNPCSubsystem`**. Au **`EndPlay`** : désinscription (toutes les listes, au cas où la catégorie aurait changé).

### IA : `ATheHouseNPCAIController`

Classe dédiée aux PNJ : c’est ici que vous assignerez plus tard **Behavior Tree**, **Blackboard**, **AI Perception**, **EQS**. Le projet utilise déjà **`UTheHouseObjectSlotUserComponent`** sur le corps du PNJ pour les interactions objet/slot sans imposer un BT minimal.

### Registre monde : `UTheHouseNPCSubsystem`

**`UWorldSubsystem`** — fonctions utiles :

- **`GetNPCsByCategory`**, **`GetNPCCountByCategory`**

Utile pour l’UI « liste du personnel », statistiques, boucles économie, sans **`TActorIterator`** à chaque frame.

### Chaînage avec les objets casino

1. Configurer les **slots** sur **`ATheHouseObject`** (voir section [IA : utilisation des slots PNJ](#ia-utilisation-des-slots-pnj)).
2. Ajouter **`UTheHouseObjectSlotUserComponent`** au Blueprint PNJ parent.
3. Depuis le BT ou Blueprint, appeler **`UseObjectSlot`** avec le bon **`PurposeTag`**.

### Compilation

Si **Live Coding** est actif, la compilation peut échouer (`Unable to build while Live Coding is active`). Fermer l’éditeur ou désactiver Live Coding / utiliser **Ctrl+Alt+F11** selon votre flux.

### Prochaines étapes (éditeur)

1. Compiler le module **`TheHouse`** (éditeur fermé ou sans Live Coding bloquant).
2. Créer des **Data Assets** du bon sous-type (**Staff / Customer / Special**) selon chaque recette (ex. `DA_Staff_Dealer`, `DA_Customer_Standard`, `DA_Special_Police`).
3. Créer un **Blueprint** basé sur **`ATheHouseNPCCharacter`** ; y assigner **mesh**, **anim**, Animation Blueprint, et le champ **`NPC Archetype`**.
4. Ajouter le composant **`TheHouse Object Slot User`** sur ce Blueprint pour les interactions avec les **`ATheHouseObject`** (réservation de slot, déplacement, montage).
5. Vérifier que le PNJ possède bien **`ATheHouseNPCAIController`** (déjà défini par défaut en C++) ou un **Blueprint** de contrôleur dérivé si vous personnalisez la BT.
6. Créer ou assigner un **Behavior Tree** + **Blackboard** sur ce contrôleur IA quand le comportement dépasse les appels simples depuis Blueprint.
7. Tester **`GetNPCsByCategory`** (Blueprints ou C++) pour valider le registre ; placer plusieurs PNJ avec des archetypes différents.
8. Brancher agrégation **`StaffMonthlySalary`** / **`DefaultMonthlySalary`** et flux **`Wallet`** vers votre **économie** lorsque prêt — le runtime expose **`StaffMonthlySalary`** et **`Wallet`**.

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
6. **UMG RTS racine en `Visible` plein écran** : bloque la sélection monde et le rectangle HUD ; utiliser **`SelfHitTestInvisible`** sur les grands conteneurs (voir section Interface RTS).
7. **Panneau paramètres qui “ne s’ouvre pas”** : vérifier d’abord **`PlacedObjectSettingsWidgetClass`** sur le BP du PlayerController, puis la collision **`Visibility`** de l’objet, puis **`bOpenParametersPanelOnLeftClick`** sur le BP de l’objet.

---

## Références doc existantes

| Document | Contenu |
|----------|---------|
| `Docs/README.md` | Index de la documentation Features / Changelog. |
| `Docs/Features/README.md` | Index des dossiers feature (couverture `Source/`). |
| `Docs/PROJECT_OVERVIEW.md` | Vision produit courte + titres de travail. |
| `Docs/SYSTEMS.md` | Liste des systèmes alignée sur le code. |
| `Docs/Features/CasinoPlaceableObjects/README.md` | Objets plaçables, workflow BP. |
| `Docs/Features/SmartWall/README.md` | Smart wall. |
| `Docs/Features/RTS_UI/README.md` | Panneau RTS, menus contextuels, ordres PNJ. |
| `Docs/Features/Localization/README.md` | Runtime + pipeline CLI. |
| `Docs/Features/NPCWorld/README.md` | Subsystem, archetypes, éjection, IA slot. |
| `Docs/Changelog/CHANGELOG.md` | Journal des versions. |

---

## Localisation (pipeline complet en ligne de commande)

Le **tableau de bord de localisation** reste nécessaire pour **créer une fois la cible « Game »** (génération des `Config/Localization/Game_*.ini`). Ensuite, utiliser **`Scripts/RunLocalizationStep.ps1`** (ou les raccourcis **`GatherLocalization.ps1`** / **`CompileLocalization.ps1`**) lance **`UnrealEditor-Cmd`** avec **`-LiveCoding=false`**. Lancer Gather/Compile/Export depuis le dashboard **pendant que l’éditeur principal est ouvert** déclenche souvent **Live Coding** (`Waiting for server`, erreurs de pipes).

**Réparation Gather :** avec `-Step Gather`, `Game_Gather.ini` est corrigé automatiquement si le dashboard a supprimé `SearchDirectoryPaths` / `IncludePathFilters`.

### Script principal : `RunLocalizationStep.ps1`

Moteur par défaut attendu par les scripts : installation Epic type `UE_5.7` (sinon `-EngineRoot`).

À lancer depuis la **racine du dépôt** (dossier qui contient `TheHouse.uproject` et le dossier `Scripts/`).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\RunLocalizationStep.ps1" -Step <NomEtape>
```

Option moteur : `-EngineRoot "D:\UE\UE_5.7"`.

| `-Step` | INI | Rôle |
|---------|-----|------|
| **`Gather`** | `Game_Gather.ini` | Collecte **C++/INI** + **assets** → manifeste + archive (+ rapports selon l’INI). |
| **`Compile`** | `Game_Compile.ini` | Génère le **texte runtime** (`.locres`, méta) à partir du manifeste / archive. |
| **`Export`** | `Game_Export.ini` | Export (ex. **`Game.po`**) pour traducteurs externes. |
| **`Import`** | `Game_Import.ini` | Réimporte le **`Game.po`** modifié dans l’archive. |
| **`ExportDialogueScript`** | `Game_ExportDialogueScript.ini` | Export **`GameDialogue.csv`** (script / lignes pour enregistrement voix ou studio). |
| **`ImportDialogueScript`** | `Game_ImportDialogueScript.ini` | Réimport du **`GameDialogue.csv`** édité. |
| **`ImportDialogue`** | `Game_ImportDialogue.ini` | **`ImportLocalizedDialogue`** — rattache les **WAV** par culture (`RawAudioPath`, `ImportedDialogueFolder` dans l’INI ; à régler dans le dashboard ou l’INI avant usage). |
| **`Reports`** | `Game_GenerateReports.ini` | Rapports / **nombre de mots** (ex. `Game.csv`). |
| **`RepairGatherIni`** | *(sans moteur)* | Répare seule `Game_Gather.ini` ; équivalent **`GatherLocalization.ps1 -RepairOnly`**. |

**Raccourcis :** `GatherLocalization.ps1` = `-Step Gather` (+ `-RepairOnly`) ; `CompileLocalization.ps1` = `-Step Compile`.

### Workflows typiques

**A — Texte seul (boucle courante)**  
1. `Gather` → 2. `Compile`.

**B — Traducteurs (PO)**  
1. `Gather` → 2. `Export` → *(traduire `Game.po`)* → 3. `Import` → 4. `Compile`.

**C — Voix / studio : script dialogue (CSV)**  
Après manifeste : `ExportDialogueScript` → édition / enregistrement selon **`GameDialogue.csv`** → `ImportDialogueScript` → **`ImportDialogue`** une fois les **WAV** aux bons chemins (`RawAudioPath`, etc.) → `Compile` si le flux l’exige.

**D — Rapports seuls**  
`Reports` après un `Gather` réussi (ou pour rafraîchir les compteurs).

### Voix et audio **en dehors** de ce jeu d’INI

- **`ImportDialogue`** couvre l’import **dialogue localisé** Unreal (WAV attendus par la config) — **pas** la génération automatique des banques **Wwise**.
- **Wwise (ou autre middleware)** : banques par langue, bascule moteur son, packaging = **processus à part** ; à documenter côté audio quand vous ajoutez la VO.
- **MetaSound / `USoundWave` par dossier culture** : chargement selon **culture** dans le jeu — orthogonal au `GatherText`, mais la **culture** peut rester celle d’`Internationalization`.

### Ligne moteur équivalente (toute étape)

Même modèle ; seul **`-config=`** change :

```text
"<UE_5.7>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<Projet>\TheHouse.uproject" -run=GatherText "-config=Config/Localization/Game_<Ini>.ini" -unattended -nop4 -nosplash -NullRHI -LiveCoding=false -log
```

Remplace `<UE_5.7>` par la racine du moteur (dossier contenant `Engine\`) et `<Projet>` par la racine du dépôt TheHouse.

| Option | Rôle |
|--------|------|
| `-run=GatherText` | Orchestrateur : lit l’INI et enchaîne les sous-commandlets. |
| `-config=…` | INI **relatif à la racine du projet**. |
| `-unattended` | Mode batch. |
| `-nop4` | Sans Perforce. |
| `-nosplash` | Pas de splash. |
| `-NullRHI` | Pas de RHI complète. |
| `-LiveCoding=false` | Évite le blocage Live Coding (deuxième processus). |
| `-log` | Logs dans `Saved/Logs/`. |

### Copier-coller rapide

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\GatherLocalization.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\CompileLocalization.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\GatherLocalization.ps1" -RepairOnly
```

Exemple **export PO** après Gather :

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\RunLocalizationStep.ps1" -Step Export
```

---

## Site web (documentation)

Une version **HTML** (navigation + thème + pages FR/EN) est disponible dans **`Docs/site/`** : `npm run dev` dans `Docs/site/`, ou ouvrir `Docs/site/index.html` dans un navigateur. Elle reprend l’architecture du guide et les liens vers les features (`Docs/Features/`).

*Dernière mise à jour : alignée sur le code du module `TheHouse` (UE 5.7) — inclut UI RTS, localisation runtime, volume d’éjection PNJ.*
