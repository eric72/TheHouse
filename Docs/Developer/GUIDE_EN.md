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
9. [RTS pawn: camera](#rts-pawn-athehousecamerapawn)
10. [FPS character](#fps-character-athehousefpscharacter)
11. [HUD & selection](#hud--selection)
12. [Casino objects: `ATheHouseObject`](#casino-objects-athehouseobject)
13. [Placement (grid, preview)](#placement-grid-preview)
14. [`ITheHouseSelectable` interface](#ithehouseselectable-interface)
15. [AI: NPC slot usage](#ai-npc-slot-usage)
16. [Smart wall](#smart-wall)
17. [Debug & console variables](#debug--console-variables)
18. [Common pitfalls](#common-pitfalls)
19. [Existing documentation](#existing-documentation)

---

## Game vision

Design summary (see also `Docs/PROJECT_OVERVIEW.md` and `Docs/SYSTEMS.md`):

- **Primary genre:** casino management / **RTS** (top-down camera, object placement).
- **Secondary activity:** **FPS** (exploration, missions). FPS **does not** run casino management.
- **Target systems:** RTS camera, grid placement, NPC staff, economy, object interaction.

Current C++ work focuses on **RTS camera**, **FPS switch**, **object placement**, **selection**, **NPC slots** on objects, and helpers (**camera occlusion**, walls, etc.).

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

- **`ATheHousePlayerController`** possesses either the **RTS pawn** (`ATheHouseCameraPawn`) or the **FPS character** (`ATheHouseFPSCharacter`). It centralizes zoom, RTS movement, placement, selection, and FPS transition.
- **`ATheHouseObject`** is a placeable actor with an **exclusion footprint** (prevents overlap), placement states, and **NPC slots** (sockets + tags).
- **`UTheHouseObjectSlotUserComponent`** is an AI helper to reserve a slot, move, align, and optionally play a montage — no Behavior Tree required.

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

---

## Existing documentation

| Document | Content |
|----------|---------|
| `Docs/README.md` | Index of Features / Changelog. |
| `Docs/PROJECT_OVERVIEW.md` | Short product vision. |
| `Docs/SYSTEMS.md` | Game systems list. |
| `Docs/Features/CasinoPlaceableObjects/README.md` | Placeable objects, BP workflow. |
| `Docs/Features/SmartWall/README.md` | Smart wall. |
| `Docs/Changelog/CHANGELOG.md` | Version history. |

---

## Web version

An **HTML** version (navigation + styling) lives under **`Docs/site/`**: open `index.html` in a browser (a local static file server is ideal if the browser blocks some relative paths).

*Last updated: aligned with the `TheHouse` module codebase (UE 5.7).*
