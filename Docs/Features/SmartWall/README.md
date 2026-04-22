# SmartWall (`ATheHouseSmartWall`)

Mur « intelligent » pour la caméra RTS : tag `SeeThroughWall`, réduction de hauteur façon Sims, paramètres matériau (`CameraFade`, `WallHalfHeight`), et option de remplissage de coupe (expérimental).

**Code source :** `Source/TheHouse/Player/TheHouseSmartWall.h` / `.cpp` · natif BP : `BP_SmartWall`

Retour : [Features — index](../README.md) · [Documentation TheHouse](../../README.md)

---

## Blueprint — où tout se trouve

Dans les **Class Defaults** ou sur une instance du BP qui hérite de `ATheHouseSmartWall`, les paramètres sont regroupés sous :

| Catégorie | Contenu principal |
|-----------|-------------------|
| **SmartWall\|Mesh** | `WallMesh`, `bAlignMeshBottomToActorOrigin`, `MeshVerticalOffset` |
| **SmartWall\|Occlusion** | `SetWallOpacity`, occlusion, hauteur minimale, Fill Cap (voir ci‑dessous) |

---

## Occlusion et hauteur du mur quand la caméra passe

- **`bShrinkMeshHeightForOcclusion`** : à `true`, le mur réduit son **scale Z** quand l’occlusion augmente (via `SetWallOpacity`).
- **`MinVisibleHeightCm`** : hauteur **minimale réelle** (cm monde) conservée à l’occlusion max. Le code calcule automatiquement le facteur d’échelle à partir de la hauteur du mesh — pas besoin de régler un pourcentage à la main.
- **`MinWallHeightScaleWhenOccluded`** : secours / avancé ; si `MinVisibleHeightCm > 0`, le comportement principal suit **`MinVisibleHeightCm`**.
- **`bCompensateCenterPivot`** : à utiliser si le pivot du mesh est au **centre** du volume ; à **`false`** si le pivot est déjà en bas (mesh importé Blender / OBJ dédié).
- **`bAlignMeshBottomToActorOrigin`** : à **`false`** pour un mesh dont le pivot est **déjà en bas** (sinon double décalage vertical).
- **`bDriveMaterialParameter`** : pousse le scalaire `CameraFade` (nom configurable dans `MaterialFadeParameterName`) sur les MID du mur.

---

## Matériau `M_SmartWall`

Le matériau pilote la découpe / dither avec `CameraFade` et `WallHalfHeight` (noms alignés sur les propriétés du SmartWall).

Pour un rendu « intérieur cohérent » (proche Sims / Big Ambitions) **sans** composant Fill Cap :

- Régler **`InteriorTexture`** (ou équivalent dans le graphe) pour que les faces **intérieures** (dos / coupe) soient une texture ou une couleur unie.
- Le **`TwoSidedSign`** dans le graphe permet de séparer recto / verso.

---

## Fill Cap (remplissage de la coupe)

**Statut : expérimental.** Un second composant (`WallFillCapMesh`) peut recouvrir la **face supérieure** du mur réduit avec un cube rescalé et le même matériau que le mur, avec `CameraFade` forcé à 0 sur le MID du cap.

**Problèmes connus :** selon le graphe de `M_SmartWall` (branches `TwoSidedSign`, `WorldAlignedTexture`, paramètres de texture), le cap peut **noircir** d’autres faces ou provoquer des artefacts. En cas de souci, **désactiver** le remplissage :

- **`Fill Cap - Enable` (`bUseFillCap`)** : `false`
- **`Fill Cap - Texture`** : vide  
- (Le cap ne s’active pas non plus si `bUseFillCap` est faux **et** `Fill Cap - Texture` est vide.)

**Réglages utiles si tu réactivais plus tard :** `Fill Cap - Thickness (cm)`, `Fill Cap - Z Overlap (cm)`, `Fill Cap - Use Interior Texture`, noms de paramètres texture (`MainTexture` / `InteriorTexture`).

---

## Meshes avec pivot en bas / au coin

Un **obj** de référence avec pivot au coin bas est fourni dans le projet :

- `Content/Meshes/SM_WallPivotBottomLeft.obj`

Après import Unreal : assigner le Static Mesh au `WallMesh`, translation `(0,0,0)`, et **`bAlignMeshBottomToActorOrigin = false`**.

---

## Références rapides

| Sujet | Fichier / emplacement |
|-------|------------------------|
| Acteur mur | `TheHouseSmartWall` |
| Changelog doc | [Changelog/CHANGELOG.md](../../Changelog/CHANGELOG.md) |

Retour : [Documentation TheHouse](../../README.md) · [Index des features](../README.md)
