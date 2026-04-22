# Developer Guide — TheHouse

Onboarding documentation for anyone taking over the C++ codebase and gameplay systems.

**Engine:** Unreal Engine **5.7** · **Module:** `TheHouse` (C++ runtime)

---

## Table of contents

1. [Game vision](#game-vision)
2. [High-level architecture](#high-level-architecture)
3. [Source tree](#source-tree)
4. [Configuration & entry point](#configuration--entry-point)
5. [Input: keys and mouse](#input-keys-and-mouse)
6. [GameMode](#gamemode-athehousegamemodebase)
7. [Viewport client (RTS mouse wheel)](#viewport-client-uthehousegameviewportclient)
8. [PlayerController (RTS / FPS)](#playercontroller-athehouseplayercontroller)
9. [RTS UI: main panel vs context menu](#rts-ui-main-panel-vs-context-menu)
10. [RTS pawn: camera](#rts-pawn-athehousecamerapawn)
11. [FPS character](#fps-character-athehousefpscharacter)
12. [HUD & selection](#hud--selection)
13. [Casino objects: `ATheHouseObject`](#casino-objects-athehouseobject)
14. [Placement (grid, preview)](#placement-grid-preview)
15. [`ITheHouseSelectable` interface](#ithehouseselectable-interface)
16. [AI: NPC slot usage](#ai-npc-slot-usage)
17. [NPC system (architecture)](#npc-system-architecture)
18. [Smart wall](#smart-wall)
19. [Debug & console variables](#debug--console-variables)
20. [Common pitfalls](#common-pitfalls)
21. [Existing documentation](#existing-documentation)
22. [Localization (full command-line pipeline)](#localization-full-command-line-pipeline)

---

## Game vision

Design summary (see also `Docs/PROJECT_OVERVIEW.md` and `Docs/SYSTEMS.md`):

- **Primary genre:** casino management / **RTS** (top-down camera, object placement).
- **Secondary activity:** **FPS** (exploration, missions). FPS **does not** run casino management.
- **Target systems:** RTS camera, grid placement, NPC staff, economy, object interaction, RTS UMG, localization.

Current C++ work focuses on **RTS camera**, **FPS switch**, **object placement**, **selection**, **RTS UI** (main panel, object / NPC / order context menus, placed-object settings), **NPC slots** on objects, an **NPC foundation** (categories, archetype Data Assets, world registry, customer **eject** volume), **localization** (subsystem + SaveGame, CLI pipeline), and helpers (**camera occlusion**, walls, etc.).

---

## High-level architecture

```
┌─────────────────────────────────────────────────────────────┐
│  ATheHouseGameModeBase                                       │
│  DefaultPawn = ATheHouseCameraPawn | PC = ATheHousePlayerController │
└──────────────────────────┬──────────────────────────────────┘
                           │
     ┌─────────────────────┼─────────────────────┐
     ▼                     ▼                     ▼
 UTheHouseGameViewportClient   ATheHouseHUD     (level Blueprints)
 (wheel → RTS zoom)            (selection rect)
```

- **`ATheHousePlayerController`** possesses either the **RTS pawn** (`ATheHouseCameraPawn`) or the **FPS character** (`ATheHouseFPSCharacter`). It centralizes zoom, RTS movement, placement, selection, and FPS transition. It **does not** drive casino NPCs (see **NPC system** below).
- **`ATheHouseObject`** is a placeable actor with an **exclusion footprint** (prevents overlap), placement states, and **NPC slots** (sockets + tags).
- **`UTheHouseObjectSlotUserComponent`** is an AI helper to reserve a slot, move, align, and optionally play a montage — no Behavior Tree required.
- **`ATheHouseNPCCharacter` + `ATheHouseNPCAIController` + `UTheHouseNPCSubsystem`:** NPC characters possessed by an **AIController**, tracked by **category**; configured via **`NPC/Archetypes/`** Data Assets (**Staff / Customer / Special**, all derived from **`UTheHouseNPCArchetype`** — root is non-abstract so the editor asset picker lists subclasses).

---

## Source tree

| Path | Role |
|------|------|
| `Source/TheHouse/TheHouse.cpp` | Primary game module (`IMPLEMENT_PRIMARY_GAME_MODULE`). |
| `Source/TheHouse/Core/TheHouseGameModeBase.*` | Default game mode (RTS pawn, PC, HUD). |
| `Source/TheHouse/Core/TheHouseGameViewportClient.*` | Intercepts **MouseWheelAxis** for zoom. |
| `Source/TheHouse/Core/TheHouseHUD.*` | RTS selection rectangle drawing. |
| `Source/TheHouse/Core/TheHouseSelectable.h` | **`ITheHouseSelectable`** interface. |
| `Source/TheHouse/Player/TheHousePlayerController.*` | **RTS/FPS**, placement, grid, selection. |
| `Source/TheHouse/Player/TheHouseCameraPawn.*` | Spring arm, RTS camera, **pitch on boom**, occlusion. |
| `Source/TheHouse/Player/TheHouseFPSCharacter.*` | FPS character + selectable. |
| `Source/TheHouse/Player/TheHouseSmartWall.*` | Wall actor (details: `Docs/Features/SmartWall/`). |
| `Source/TheHouse/Object/TheHouseObject.*` | Placeable casino object, footprint, NPCs. |
| `Source/TheHouse/Placement/TheHousePlacementTypes.h` | `EPlacementState` (None / Previewing / Placing). |
| `Source/TheHouse/AI/TheHouseObjectSlotUserComponent.*` | “Premium” AI helper for object slots. |
| `Source/TheHouse/NPC/TheHouseNPCTypes.h` | `ETheHouseNPCCategory`, `ETheHouseNPCSpecialKind` (police, firefighter, doctor, …). |
| `Source/TheHouse/NPC/Archetypes/TheHouseNPCArchetype.*` | Root **`UTheHouseNPCArchetype`** plus **`…Staff` / `…Customer` / `…Special`** — category-specific fields. |
| `Source/TheHouse/NPC/TheHouseNPCIdentity.h` | Interface **`ITheHouseNPCIdentity`** (`GetNPCCategory`, `GetNPCArchetype`). |
| `Source/TheHouse/NPC/TheHouseNPCCharacter.*` | **`ATheHouseNPCCharacter`** — runtime state (`Wallet`), `ApplyArchetypeDefaults`, AI possession. |
| `Source/TheHouse/NPC/TheHouseNPCAIController.*` | **`ATheHouseNPCAIController`** — hook for BT / perception (extend as needed). |
| `Source/TheHouse/NPC/TheHouseNPCSubsystem.*` | **`UTheHouseNPCSubsystem`** — category registry in the world (`GetNPCsByCategory`). |
| `Source/TheHouse/NPC/TheHouseNPC.h` | Optional umbrella include for the NPC module. |
| `Source/TheHouse/NPC/TheHouseNPCEjectRegionVolume.*` | Box volume: region from which a **customer eject** order can be issued; exit point along dominant face (`TryComputeEjectionExitWorldLocation`). |
| `Source/TheHouse/Localization/TheHouseLocalizationSubsystem.*` | Active culture, applies to text widgets. |
| `Source/TheHouse/Localization/TheHouseLanguageSaveGame.*` | Persists language choice (`USaveGame`). |
| `Source/TheHouse/UI/TheHouseRTSMainWidget.*` | Main RTS panel (money, catalog, stock) — `BindWidgetOptional` on `MoneyText`, `CatalogScroll`, `StoredScroll`. |
| `Source/TheHouse/UI/TheHouseRTSContextMenuUMGWidget.*` / `TheHouseRTSContextMenuWidget.*` | **Placed object** context menu (UMG or pure Slate). |
| `Source/TheHouse/UI/TheHouseNPCRTSContextMenuUMGWidget.*` | **NPC** context menu (no prior selection). |
| `Source/TheHouse/UI/TheHouseNPCOrderContextMenuUMGWidget.*` | **NPC orders** menu (ground / object / NPC) when NPCs are selected. |
| `Source/TheHouse/UI/TheHousePlacedObjectSettingsWidget.*` | Settings panel for a placed `ATheHouseObject`. |
| `Source/TheHouse/UI/TheHouseFPSHudWidget.*` | FPS-side UMG HUD (when wired). |
| `Source/TheHouse/UI/TheHouseRTSUIClickRelay.*` | Forwards catalog / stock clicks to the Player Controller. |
| `Source/TheHouse/Player/BP_SmartWall.*` | **Blueprint native** class for the smart wall (see `Docs/Features/SmartWall/`). |

**Blueprints:** the map’s default game mode is often `BP_HouseGameMode` (`Config/DefaultEngine.ini`). It **must** inherit from or use the project’s C++ classes so input fixes and the custom PC apply.

---

## Configuration & entry point

- **`TheHouse.uproject`:** `TheHouse` module; **Enhanced Input plugin disabled** — the project relies on **legacy Action/Axis mappings** (`DefaultInput.ini`) and **workarounds** (input timer, viewport) when focus or UI block keys.
- **`Config/DefaultEngine.ini`:**
  - `GameViewportClientClassName=/Script/TheHouse.TheHouseGameViewportClient` — required so **mouse wheel** is handled before Game+UI issues.
  - `GlobalDefaultGameMode`: often a Blueprint; ensure **Player Controller Class** stays compatible with `ATheHousePlayerController` (see GameMode).

---

## Input: keys and mouse

**Named** bindings (Action / Axis) live in **`Config/DefaultInput.ini`**. **`ATheHousePlayerController`** forces a classic **`UInputComponent`** in `SetupInputComponent()` (the INI may still list `DefaultPlayerInputClass=EnhancedPlayerInput`; the C++ path overrides so `BindAction` / `BindAxis` work).

### Actions

| Action name | Default keys (`DefaultInput.ini`) | C++ binding | Role |
|-------------|-------------------------------------|-------------|------|
| `DebugToggleFPS` | **F** | `BindAction` → `DebugSwitchToFPS` | RTS: switch to FPS at cursor ground hit. FPS: switch back to RTS. |
| `RotateModifier` | **Left Alt**, **Right mouse button** | Pressed / Released | Camera-rotate modifier: while active, left click does **not** start selection or confirm placement. |
| `Select` | **Left mouse button** | Pressed / Released | RTS: selection box start/end; in **placement** preview: confirms placement on click (see LMB fallback below). |
| `DebugPlaceObject` | **P** | Pressed | RTS: start **placement** preview if `PendingPlacementClass` is set; if preview already active, **cancels** placement. |

### Axes

| Axis name | Default inputs (`DefaultInput.ini`) | C++ binding | Role |
|-----------|-------------------------------------|-------------|------|
| `MoveForward` | **Z**, **W** (+ forward) · **S** (− back) | `MoveForward` | RTS plane movement (camera pawn). |
| `MoveRight` | **D** (+ right) · **Q**, **A** (− left) | `MoveRight` | Same. |
| `Turn` | **MouseX** | `RotateCamera` | RTS yaw (controller). |
| `LookUp` | **MouseY** | `LookUp` | FPS: camera pitch; RTS pitch is mainly on the **spring arm** in `ATheHouseCameraPawn`. |
| `Zoom` | **MouseWheelAxis** | `Zoom` | RTS zoom (spring arm length). **Note:** part of zoom also goes through `UTheHouseGameViewportClient` → `TheHouse_ApplyWheelZoom` for reliable wheel in Game+UI. |

### Placement preview (RTS): wheel — zoom vs rotate (Shift)

When: **`EPlacementState::Previewing`**, possessed pawn is **`RTSPawnReference`** (RTS camera).

The **preview** is the **`ATheHouseObject`** actor spawned for placement preview (**`PreviewActor`** on `ATheHousePlayerController`): the same object type you are about to place, before you confirm.

| Input | Effect |
|-------|--------|
| **Wheel without Shift** | **Zoom in/out** on the camera (spring arm): same as outside placement — `Zoom` axis + `TheHouse_ApplyWheelZoom` from the viewport. |
| **Shift (left or right) + wheel** | **No zoom**: `TheHouse_ApplyWheelZoom` **returns early** when Shift is held. Rotation applies to that preview **`ATheHouseObject`**: in **`PlayerTick`**, **`WasInputKeyJustPressed(MouseScrollUp)`** / **`MouseScrollDown`** steps the **yaw** of that actor by **`PlacementRotationStepDegrees`** per notch (default **15°**, editable on `ATheHousePlayerController`). |

**Note:** this is not an optical “zoom on the pivot” — it **rotates** that **`ATheHouseObject`** placement ghost on the ground around the vertical axis (**yaw**).

### Other code-only behavior

| Behavior | Detail |
|----------|--------|
| **Left click fallback** | In `PlayerTick`, if placement preview is active: confirms on **first detected** left-button press (backup if Action Mappings misbehave). |
| **Input poll timer (~120 Hz)** | `TheHouse_PollInputFrame` physically polls **Z, Q, S, D, W, A** via the viewport when normal axes fail (Slate focus, etc.). |

To change keys: **Project → Project Settings → Input**, or edit `DefaultInput.ini`, keeping **names** like `MoveForward`, `Select`, etc., so C++ bindings stay valid.

---

## GameMode (`ATheHouseGameModeBase`)

Files: `TheHouseGameModeBase.h/.cpp`

- **`DefaultPawnClass`** = `ATheHouseCameraPawn` (start in RTS).
- **`PlayerControllerClass`** = `ATheHousePlayerController`.
- **`HUDClass`** = `ATheHouseHUD`.

**Important — `InitGame`:** if a Blueprint game mode leaves the player controller on Epic’s default, the code **forces** `ATheHousePlayerController` and logs an **error**. Without that, WASD, zoom, placement, and timers may appear broken.

---

## Viewport client (`UTheHouseGameViewportClient`)

Files: `TheHouseGameViewportClient.h/.cpp`

- Overrides **`InputAxis`** for **`EKeys::MouseWheelAxis`**.
- In **Game+UI**, wheel events often never reach `BindAxis`; here we call **`TheHouse_ApplyWheelZoom`** on `ATheHousePlayerController` and return **`true`** without calling `Super` for that case — avoids **double zoom**.

---

## PlayerController (`ATheHousePlayerController`)

Files: `TheHousePlayerController.h/.cpp`

### Main responsibilities

1. **RTS pawn** reference `RTSPawnReference` and **FPS** `ActiveFPSCharacter`.
2. **RTS input:** forward/right on the plane (axes), **zoom** (spring arm length), **yaw** rotation (controller) and **pitch** (via **`ATheHouseCameraPawn`** boom, not controller pitch alone).
3. **Timer `TheHouse_PollInputFrame` (~120 Hz):** workaround when `PlayerInput`, Slate focus, or a Blueprint **Event Tick** without **Parent** breaks axes. Physical key polling (WASD) via viewport state.
4. **Placement:** `DebugStartPlacement`, `UpdatePlacementPreview`, `ConfirmPlacement`, `CancelPlacement`, `EPlacementState`, **`PreviewActor`**.
5. **Grid:** `FTheHousePlacementGridSettings` (`CellSize`, `WorldOrigin`, `MinUpNormalZ`, `bEnableGridSnap`), runtime cache **`OccupiedGridCells`**.
6. **Selection:** mouse drag + HUD; on release, actors in the rectangle implementing **`ITheHouseSelectable`** receive **`OnSelect`**.

### RTS selection: marquee, UMG, parameters panel

- **`Input_SelectPressed` / `Input_SelectReleased`** (`TheHousePlayerController.cpp`): in RTS (pawn = camera), **left click** starts **`bIsSelecting`**; timer **`TheHouse_PollInputFrame`** calls **`ATheHouseHUD::UpdateSelection`** for the green marquee.
- **`Input_SelectDoubleClick`**: the PlayerController also binds **`EKeys::LeftMouseButton` + `IE_DoubleClick`**. This is a fallback when the **second click** is swallowed by UMG: a double-click on an object can still run a world trace and reopen the parameters panel.
- **`bRtsSelectGestureFromWorld`:** set **true** only if the press actually started this drag from “world” input (no early-return). On **release**, if **false**, all marquee / single-click selection logic is skipped (avoids using a stale **`SelectionStartPos`** after a UI click).
- **`TheHouse_IsCursorOverBlockingRTSViewportUI`:** whether the cursor is over UMG that **must** consume the click instead of the world. **Context menu** and **placed-object settings** use widget **geometry** (`IsUnderLocation`). The **main RTS panel** no longer relies on root **geometry alone** (often fullscreen): Slate **widget path** under the cursor + UMG **`Visible`** (hit-testable) in that WBP tree; **`SelfHitTestInvisible`** / **`HitTestInvisible`** regions let clicks reach the world (marquee, object picks).
- **Parameters panel:** **`PlacedObjectSettingsWidgetClass`** on the Player Controller Blueprint (usually **`BP_HouseController`**); on the object **`bOpenParametersPanelOnLeftClick`** (+ economy **`bNpcMoneyFlowEnabled`**, **`NpcSpendToPlay`**, **`NpcMaxReturnToHouse`**). Opened via **`OpenPlacedObjectSettingsWidgetFor`** after selection; close with **`ClosePlacedObjectSettingsPanel`** (Blueprint), Escape (**`TryConsumeEscapeForRtsUi`**), or sell from the menu. A **second click** on the **same** object refreshes the existing WBP (**`SetTargetPlacedObject`**) without recreating it.
- **Blueprint checklist:** in **`BP_HouseController`**, set **`PlacedObjectSettingsWidgetClass`**, **`RTS Main Widget Class`**, and **`RTS Context Menu Widget Class`** if you use custom WBPs. In each casino object BP, verify **`bOpenParametersPanelOnLeftClick`**, **`PurchasePrice`**, and **`SellValue`**.

### RTS ↔ FPS switch

- **`SwitchToRTS()`:** cancels placement, syncs RTS camera to FPS position (if height is reasonable), repossesses RTS pawn, **destroys** FPS character, cursor visible, `FInputModeGameAndUI`.
- **`SwitchToFPS(TargetLocation, TargetRotation)`:** spawns `FPSCharacterClass` (default: C++ class), possesses, hides obvious **test cube** meshes, `FInputModeGameOnly`, timer to restore gravity/falling.
- **`DebugSwitchToFPS()`:** from RTS, **line trace** from mouse (deproject) to find **ground**; fallback to RTS pivot if needed.

### Mouse wheel zoom

- Settings: `RTSWheelZoomMultiplier`, `RTSZoomSpeed`, `RTSMinArmDeltaPerWheelEvent` (avoids tiny steps on high-DPI mice), `bRTSZoomScalesWithArmLength`, `RTSCursorZoomMaxPanPerEvent`, `RTSZoomInterpSpeed`, etc.
- During **placement:** **Shift + wheel** = **preview rotation** (no zoom); handled in **`PlayerTick`**.

### Placement preview flow

- Trace under cursor → “floor” surface (`ValidatePlacement`: normal Z ≥ `MinUpNormalZ`).
- Optional XY snap, yaw = `PlacementPreviewYawDegrees`.
- **`ATheHouseObject::EvaluatePlacementAt`:** overlap with other objects + world.
- Grid: approximate **exclusion box** footprint projected to cells; tested against `OccupiedGridCells`.

---

## RTS UI: main panel vs context menu

`ATheHousePlayerController` references **two** different widget types (not a duplicate):

| Property (usually in the Player Controller Blueprint **Class Defaults**, e.g. `BP_HouseController`) | Reference C++ class | Role |
|------------------------------------------------------------------------------------------------------|---------------------|------|
| **`RTS Main Widget Class`** | `UTheHouseRTSMainWidget` | **Persistent RTS panel:** money, placement catalog, stock (counts per type). Shown while the player is in **RTS mode**. |
| **`RTS Context Menu Widget Class`** | `UTheHouseRTSContextMenuWidget` (pure Slate) or `UTheHouseRTSContextMenuUMGWidget` / child WBP | **Context menu** on **right-click** on a placed **`ATheHouseObject`** (e.g. *Sell*, *Store to stock*). Temporary popup under the cursor, closed after the action. |

**Customizing the main panel:** create a Widget Blueprint parented to **`TheHouse RTS Main Widget`**, add three widgets named exactly **`MoneyText`**, **`CatalogScroll`**, **`StoredScroll`** (C++ `BindWidgetOptional`), then assign that WBP to **`RTS Main Widget Class`**. Details and pitfalls: comments in `Source/TheHouse/UI/TheHouseRTSMainWidget.h`.

**Thumbnails:** in **`PlaceableCatalog`** (Player Controller Class Defaults), each entry can set **`Thumbnail`** (`Texture2D`); the RTS panel shows the image next to the label in both **catalog** and **stock** when the same **`ObjectClass`** exists in the catalog.

**Show lists only after clicking an icon:** C++ always fills `CatalogScroll` / `StoredScroll`; to **hide them by default**, parent them under a **`Border`** / **`WidgetSwitcher`** / panel and toggle **`Visibility`** (Collapsed / Visible) from the WBP **graph** (icon button → `SetVisibility`), e.g. after **After RTS Main Refreshed** if you need to resync state.

**`None` on `RTS Main Widget Class`:** at runtime, if the property is empty, `InitializeModeWidgets()` assigns the default C++ class `UTheHouseRTSMainWidget`.

**Hit-testing vs selection:** a fullscreen root **Canvas / Border** set to **`Visible`** still counts as “over UI” (blocks world). For decorative fullscreen chrome, prefer **`SelfHitTestInvisible`** (or **`HitTestInvisible`**); keep **`Visible`** for real controls (buttons, lists, text).

### NPC menus and orders (left click = select, right click = menu / order)

Three distinct RTS menus exist:

| Case | UI class (WBP parent) | Property on the Player Controller Blueprint |
|------|------------------------|-----------------------------------------------|
| **NPC “inspect / use…” menu** (right-click on an NPC **with no** NPC selected) | `UTheHouseNPCRTSContextMenuUMGWidget` | `NPCRTSContextMenuWidgetClass` |
| **NPC order menu (object / NPC / ground target)** (right-click **with at least one NPC selected** via left click) | `UTheHouseNPCOrderContextMenuUMGWidget` | `NPCOrderContextMenuWidgetClass` |
| **Placed object menu** (right-click on an `ATheHouseObject` with no NPC selected) | `UTheHouseRTSContextMenuUMGWidget` | `RTSContextMenuWidgetClass` |

Optional UMG widgets (`BindWidgetOptional`) in menu WBPs:

- `MenuBorder` (`UBorder`)
- `OptionsBox` (`UVerticalBox`)

### Adding order actions (C++ example)

Actions are **dynamic**: you add them when the menu opens via BlueprintNativeEvent hooks.

- `GatherNPCOrderActionsOnGround(OutOptionDefs)` — orders on the ground (clicked point).
- `ExecuteNPCOrderOnGroundAction(OptionId)` — runs the option for selected NPCs. The point is available via `GetLastNPCOrderGroundTarget()`.

Example (in a C++ subclass of `ATheHousePlayerController`):

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
	OutOptionDefs.Add({ MoveTo, FText::FromString(TEXT("Move here")) });
	OutOptionDefs.Add({ RunTo,  FText::FromString(TEXT("Run here")) });
	OutOptionDefs.Add({ DebugShout, FText::FromString(TEXT("Shout (debug)")) });
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
				UE_LOG(LogTemp, Warning, TEXT("[Order] %s shout!"), *Npc->GetName());
			}
		}
	}
}
```

---

## RTS pawn (`ATheHouseCameraPawn`)

Files: `TheHouseCameraPawn.h/.cpp`

- **`USpringArmComponent`** + **`UCameraComponent`** + **`UFloatingPawnMovement`**.
- **RTS pitch:** stored in **`RTSArmPitchDegrees`** and applied to the boom (**`SyncRTSArmPitchToBoom`**), not only control rotation — avoids fighting yaw.
- **Occlusion:** `CheckCameraOcclusion` — material parameter fade on actors between camera and pawn (e.g. `CameraFade`), sweep, anti-flicker logic.

---

## FPS character (`ATheHouseFPSCharacter`)

Files: `TheHouseFPSCharacter.h/.cpp`

- Inherits **`ACharacter`**, implements **`ITheHouseSelectable`**.
- **`bCanBeSelected`:** can disable RTS selection.
- Movement uses pressed/released bools wired to project mappings.

---

## HUD & selection

- **`ATheHouseHUD`:** `StartSelection` / `UpdateSelection` / `StopSelection` / `DrawHUD` for the rectangle.
- Which actors are selected is resolved in the **PlayerController** (screen-space / selection logic).
- UMG filtering + **`bRtsSelectGestureFromWorld`:** see **PlayerController → RTS selection** above.

---

## Casino objects (`ATheHouseObject`)

Files: `TheHouseObject.h/.cpp`

### Components

- **`ObjectMesh`:** main mesh (collision, nav).
- **`ExclusionZoneVisualizer`:** preview volume (often engine cube scaled), **no collision** for preview.

### Exclusion zone

- **`PlacementExclusionHalfExtent`**, **`PlacementExclusionCenterOffset`:** local box; **`GetWorldPlacementExclusionBox`** for tests.
- **`TestFootprintOverlapsOthersAt`:** box intersection with other `ATheHouseObject` instances.
- **`TestFootprintWorldAt`:** world blocking (ignores floor actor under cursor when set).

### States (`EObjectPlacementState`)

`Valid`, `OverlapsObject`, `OverlapsWorld`, `OverlapsGrid` — drive valid/invalid materials.

### NPCs

- **`FTheHouseNPCInteractionSlot`:** `SocketName`, `PurposeTag`.
- **`TryReserveNPCSlot`**, **`ReleaseNPCSlotByIndex`**, **`GetNPCSlotWorldTransform`**, etc. Occupants tracked in **`NPCSlotOccupants`**.

### Selection (`ITheHouseSelectable`)

- **`OnSelect` / `OnDeselect`:** highlight (**Custom Depth** / stencil), etc.

### Parameters panel (casino)

- **`bOpenParametersPanelOnLeftClick`:** when enabled on the object BP, an RTS **left click** that selects that object opens the WBP (**`PlacedObjectSettingsWidgetClass`** on the player controller).
- **NPC economy (WBP display):** **`bNpcMoneyFlowEnabled`**, **`NpcSpendToPlay`**, **`NpcMaxReturnToHouse`** — read in UI from **After Target Placed Object Changed** (see `TheHousePlacedObjectSettingsWidget.h`).
- **Sell value:** **`SellValue`** is used first when running **Sell**. If **`SellValue == 0`**, code now falls back to **`PurchasePrice`**. In practice: set **`SellValue`** only when resale must differ from buy price; otherwise leave **0** to mean “reuse purchase price”.

---

## Placement (grid, preview)

- Types: `Source/TheHouse/Placement/TheHousePlacementTypes.h` — **`EPlacementState`** for **PlayerController** flow (None, Previewing, Placing).
- **`ConfirmPlacement`:** validates, **`MarkCellsOccupied`**, **`FinalizePlacement`**, resets state.
- Console: **`TheHouse.Placement.Debug`** (see Debug section).

---

## `ITheHouseSelectable` interface

File: `TheHouseSelectable.h`

- **`OnSelect`**, **`OnDeselect`**, **`IsSelectable`** (default true).
- Used by **`ATheHouseObject`**, **`ATheHouseFPSCharacter`**, and any Blueprint/C++ actor implementing the interface.

---

## AI: NPC slot usage

Files: `TheHouseObjectSlotUserComponent.h/.cpp`

- States: **`EObjectSlotUseState`** (Idle, MovingToSlot, Aligned, PlayingMontage).
- **`UseObjectSlot(TargetObject, PurposeTag, Montage)`:** reserves slot, **`MoveTo`** via AIController, interpolated align, optional montage.
- Tunables: `AcceptableRadius`, `AlignInterpSpeed`.

---

## NPC system (architecture)

### Not the PlayerController

Casino **NPCs** are not human players; they should **not** be driven by `ATheHousePlayerController`. In Unreal, an NPC is typically an **`ACharacter`** possessed by an **`AAIController`** — here **`ATheHouseNPCAIController`**. The Player Controller remains for the human player (RTS / FPS).

### Categories (`ETheHouseNPCCategory`)

| Value | Purpose |
|--------|---------|
| **`Unassigned`** | Missing or invalid archetype — fix in data. |
| **`Staff`** | Employees: default salary (`DefaultMonthlySalary` on Staff archetype) → copied to **`StaffMonthlySalary`** on the pawn (**editable via UI**). |
| **`Customer`** | Guests: **`StartingWallet`** → **`Wallet`**; **`OptionalAdmissionFee`** lives only on Customer archetypes (0 = off; future admission gameplay). |
| **`Special`** | Police / EMS / scripted: **`ETheHouseNPCSpecialKind`** + **`SpecialDutyId`** on Special archetype. |

The category feeds the **registry**, UI, and rules. One **C++ subclass per category** keeps unrelated fields off each asset (no admission fee on staff or police archetypes).

### Data: `Source/TheHouse/NPC/Archetypes/`

- **`UTheHouseNPCArchetype`** (root; avoid using alone in gameplay) — **`DisplayName`**, **`Category`** (set by subclasses). Prefer Staff / Customer / Special `.uasset` files.
- **`UTheHouseNPCStaffArchetype`** — **`StaffRoleId`**, **`DefaultMonthlySalary`**, presentation (portrait, accent color).
- **`UTheHouseNPCCustomerArchetype`** — **`StartingWallet`**, **`OptionalAdmissionFee`**.
- **`UTheHouseNPCSpecialArchetype`** — **`SpecialKind`**, **`SpecialDutyId`**.

Create `.uasset` instances from the matching type (**TheHouse NPC Staff Archetype**, etc.). **`NPCArchetype`** on the character remains **`UTheHouseNPCArchetype*`** (polymorphism).

### Identity: `ITheHouseNPCIdentity`

Lets gameplay and UI query an actor without hard-coding **`ATheHouseNPCCharacter`**:

- **`GetNPCCategory`**
- **`GetNPCArchetype`**

### Character: `ATheHouseNPCCharacter`

- **`NPCArchetype`** — reference to the Data Asset (set on BP or placed instance).
- **`Wallet`** — customers; **`TrySpendFromWallet`** / **`AddToWallet`**.
- **`StaffMonthlySalary`** — staff; initialized from **`DefaultMonthlySalary`**; **`SetStaffMonthlySalary`** for player UI.
- **`ApplyArchetypeDefaults`** — *BlueprintNativeEvent*: fills **`Wallet`** or **`StaffMonthlySalary`** via `Cast`; call **Parent** if you override in Blueprint.
- **`BeginPlay`:** apply defaults, then **`RegisterNPC`** on **`UTheHouseNPCSubsystem`**. **`EndPlay`:** unregister from **all** internal lists (safe if category changed).

### AI: `ATheHouseNPCAIController`

Dedicated NPC controller: assign **Behavior Tree**, **Blackboard**, **AI Perception**, **EQS** here when you need them. The project already uses **`UTheHouseObjectSlotUserComponent`** on the pawn for slot/object flows without requiring a minimal BT.

### World registry: `UTheHouseNPCSubsystem`

**`UWorldSubsystem`** helpers:

- **`GetNPCsByCategory`**, **`GetNPCCountByCategory`**

Useful for staff lists, stats, economy loops without a full **`TActorIterator`** each time.

### Wiring to casino objects

1. Configure **slots** on **`ATheHouseObject`** (see [AI: NPC slot usage](#ai-npc-slot-usage)).
2. Add **`UTheHouseObjectSlotUserComponent`** to the NPC Blueprint parent.
3. From BT or Blueprint, call **`UseObjectSlot`** with the correct **`PurposeTag`**.

### Builds

If **Live Coding** is active, builds may fail (`Unable to build while Live Coding is active`). Close the editor or disable Live Coding / use **Ctrl+Alt+F11** per your workflow.

### Next steps (editor)

1. Compile the **`TheHouse`** module (editor closed or Live Coding not blocking).
2. Create **Data Assets** of the correct subclass (**Staff / Customer / Special**) per recipe (e.g. `DA_Staff_Dealer`, `DA_Customer_Standard`, `DA_Special_Police`).
3. Create a **Blueprint** parented from **`ATheHouseNPCCharacter`**; assign **mesh**, **animation**, AnimBP, and **`NPC Archetype`**.
4. Add the **`TheHouse Object Slot User`** component on that Blueprint for **`ATheHouseObject`** interactions (slot reservation, move, montage).
5. Ensure the pawn uses **`ATheHouseNPCAIController`** (already the C++ default) or a **Blueprint** child if you customize BT wiring.
6. Assign a **Behavior Tree** + **Blackboard** on the AI Controller when behavior goes beyond simple Blueprint calls.
7. Test **`GetNPCsByCategory`** (Blueprint or C++) with several NPCs using different archetypes.
8. Aggregate **`StaffMonthlySalary`** / **`DefaultMonthlySalary`** and customer **`Wallet`** flows into your **economy** when ready — runtime exposes **`StaffMonthlySalary`** and **`Wallet`**.

---

## Smart wall

Actor **`ATheHouseSmartWall`** and detailed notes: **`Docs/Features/SmartWall/README.md`** (Fill Cap, materials, etc.).

---

## Debug & console variables

| CVar | Effect |
|------|--------|
| `TheHouse.Placement.Debug` | Placement flow logs (0 = off, 1 = on). |
| `TheHouse.FPS.Debug` | Forward trace / mesh logs in FPS (0/1). |

The PlayerController also logs useful **BeginPlay** info (`PlayerInput` class, Enhanced Input warning).

---

## Common pitfalls

1. **Blueprint game mode** not using `ATheHousePlayerController` → `InitGame` fixes it but align the BP to avoid confusion.
2. **`DefaultInput.ini` switched back to Enhanced Input** while the plugin is off → broken input; startup logs warn about this.
3. **Blueprint PC / Pawn** with **Event Tick** not calling **Parent** → C++ controller tick may not run as expected.
4. **Double zoom:** do not add a second wheel path alongside `GameViewportClient`.
5. **Placement:** steep surfaces rejected (`MinUpNormalZ`); no hit under cursor = invalid.
6. **Fullscreen RTS root UMG set to `Visible`:** blocks world selection and the HUD marquee; use **`SelfHitTestInvisible`** on large containers (see RTS UI section).
7. **Parameters panel “does not open”:** first verify **`PlacedObjectSettingsWidgetClass`** on the Player Controller BP, then the object’s **`Visibility`** collision, then **`bOpenParametersPanelOnLeftClick`** on the object BP.

---

## Existing documentation

| Document | Content |
|----------|---------|
| `Docs/README.md` | Index of Features / Changelog. |
| `Docs/Features/README.md` | Feature folder index (`Source/` coverage). |
| `Docs/PROJECT_OVERVIEW.md` | Short product vision + working titles. |
| `Docs/SYSTEMS.md` | Systems list aligned with code. |
| `Docs/Features/CasinoPlaceableObjects/README.md` | Placeable objects, BP workflow. |
| `Docs/Features/SmartWall/README.md` | Smart wall. |
| `Docs/Features/RTS_UI/README.md` | RTS panel, context menus, NPC orders. |
| `Docs/Features/Localization/README.md` | Runtime + CLI pipeline. |
| `Docs/Features/NPCWorld/README.md` | Subsystem, archetypes, eject, slot AI. |
| `Docs/Changelog/CHANGELOG.md` | Version history. |

---

## Localization (full command-line pipeline)

The **Localization Dashboard** is still required to **create the “Game” target** once (generates `Config/Localization/Game_*.ini`). After that, prefer **`Scripts/RunLocalizationStep.ps1`** (or the thin wrappers **`GatherLocalization.ps1`** / **`CompileLocalization.ps1`**) so **`UnrealEditor-Cmd`** runs with **`-LiveCoding=false`**. Running Gather/Compile/Export from the dashboard **while the main editor is open** often triggers **Live Coding** (`Waiting for server`, pipe errors).

**Gather repair:** when `-Step Gather` runs, `Game_Gather.ini` is auto-patched if the dashboard removed `SearchDirectoryPaths` / `IncludePathFilters`.

### Master script: `RunLocalizationStep.ps1`

Scripts default to a typical Epic **`UE_5.7`** install unless you pass **`-EngineRoot`**.

Run from the **repository root** (folder that contains `TheHouse.uproject` and the `Scripts/` directory).

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\RunLocalizationStep.ps1" -Step <StepName>
```

Optional engine root: append `-EngineRoot "D:\UE\UE_5.7"`.

| `-Step` | Config INI | What it does (Epic commandlet chain from INI) |
|---------|------------|-----------------------------------------------|
| **`Gather`** | `Game_Gather.ini` | Collect strings from **C++/INI** and **assets** → manifest + archive (+ reports per INI). |
| **`Compile`** | `Game_Compile.ini` | Build runtime text (**`.locres`**, metadata) from manifest/archive. |
| **`Export`** | `Game_Export.ini` | Export translations (e.g. **`Game.po`**) for external translation. |
| **`Import`** | `Game_Import.ini` | Import edited **`Game.po`** back into the archive. |
| **`ExportDialogueScript`** | `Game_ExportDialogueScript.ini` | Export **`GameDialogue.csv`** — script/lines for voice recording or studio handoff. |
| **`ImportDialogueScript`** | `Game_ImportDialogueScript.ini` | Re-import **`GameDialogue.csv`** after edits. |
| **`ImportDialogue`** | `Game_ImportDialogue.ini` | **`ImportLocalizedDialogue`** — map **recorded WAVs** per culture into the dialogue pipeline (`RawAudioPath`, `ImportedDialogueFolder` in INI; adjust in dashboard or INI before use). |
| **`Reports`** | `Game_GenerateReports.ini` | Regenerate **word-count / reports** (e.g. `Game.csv`) from current manifest data. |
| **`RepairGatherIni`** | *(no engine run)* | Only fixes `Game_Gather.ini` paths; same as **`GatherLocalization.ps1 -RepairOnly`**. |

**Wrappers (optional):** `GatherLocalization.ps1` = `-Step Gather` (+ `-RepairOnly`); `CompileLocalization.ps1` = `-Step Compile`.

### Typical workflows

**A — In-game text only (day-to-day)**  
1. `Gather` → 2. `Compile`.  
Repeat whenever you change `NSLOCTEXT` / asset text.

**B — External translators (PO)**  
1. `Gather` → 2. `Export` → *(translate `Game.po`)* → 3. `Import` → 4. `Compile`.

**C — Voice / studio: dialogue script (CSV)**  
After manifest exists: `ExportDialogueScript` → edit **`GameDialogue.csv`** / record lines → `ImportDialogueScript` → then `ImportDialogue` **once WAVs are on disk** at the paths expected by `Game_ImportDialogue.ini` (`RawAudioPath`, etc.). Finish with `Compile` if your pipeline requires updated loc resources.

**D — Reports only**  
`Reports` after a successful `Gather` (or when you need refreshed CSV counts).

### Voices and audio **outside** this INI set

- **Unreal dialogue import** (`ImportDialogue`) covers **localized dialogue assets** driven by Epic’s dialogue / WAV import rules — **not** automatic Wwise bank generation.
- **Wwise (or other middleware):** language banks, sound engine switch, and packaging are **separate**; document your Wwise **language / bank** strategy in the audio chapter of your project when you add VO.
- **MetaSound / plain `USoundWave` per folder** (`.../fr/`, `.../en/`): load by **culture** in game code or data — orthogonal to `GatherText`, but you can still use **culture** from the same `Internationalization` settings.

### Equivalent engine line (any step)

Same pattern; only **`-config=`** changes:

```text
"<UE_5.7>\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "<Project>\TheHouse.uproject" -run=GatherText "-config=Config/Localization/Game_<StepIni>.ini" -unattended -nop4 -nosplash -NullRHI -LiveCoding=false -log
```

Replace `<UE_5.7>` with your engine root (folder that contains `Engine\`) and `<Project>` with your TheHouse repo root.

| Flag | Purpose |
|------|---------|
| `-run=GatherText` | Orchestrator commandlet: reads the INI and runs the listed localization sub-commandlets. |
| `-config=…` | INI path **relative to project root** (scripts `cd` to the project). |
| `-unattended` | Non-interactive batch behavior. |
| `-nop4` | No Perforce integration for this run. |
| `-nosplash` | No splash screen. |
| `-NullRHI` | Headless-friendly (no full GPU stack). |
| `-LiveCoding=false` | Avoids Live Coding broker when a second editor process starts. |
| `-log` | Standard log output under `Saved/Logs/`. |

### Copy-paste shortcuts

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\GatherLocalization.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\CompileLocalization.ps1"
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\GatherLocalization.ps1" -RepairOnly
```

Example **export PO** after Gather:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File "Scripts\RunLocalizationStep.ps1" -Step Export
```

---

## Web version

An **HTML** site (navigation + FR/EN pages) lives under **`Docs/site/`**: run `npm run dev` inside `Docs/site/`, or open `Docs/site/index.html`. It mirrors this guide’s structure and links to `Docs/Features/`.

*Last updated: aligned with the `TheHouse` module (UE 5.7) — includes RTS UI, runtime localization, NPC eject volume.*
