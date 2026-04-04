# Objets casino — placement, zones et PNJ

Documentation de reprise rapide pour le système d’objets décor / gameplay (tables, machines, billard, etc.) basé sur **`ATheHouseObject`**.

> Retour : [Documentation TheHouse](../../README.md)

---

## 1. Fichiers sources

| Fichier | Rôle |
|---------|------|
| `Source/TheHouse/Object/TheHouseObject.h` | Classe acteur, propriétés, API Blueprint |
| `Source/TheHouse/Object/TheHouseObject.cpp` | Logique (overlap, visualiseur, highlight) |
| `Source/TheHouse/Core/TheHouseSelectable.h` | Interface sélection (`OnSelect` / `OnDeselect`) |

---

## 2. Idée générale (schéma rouge / vert)

- **Mesh de l’objet** (ton « carré rouge ») : le visuel normal (static / skeletal mesh dans le BP).
- **Zone d’exclusion** (ton « vert ») : volume **plus grand** que le mesh, souvent **décalé** (ex. place devant pour un PNJ). Aucun autre `ATheHouseObject` ne doit **croiser** cette boîte en monde.

Les collisions logiques utilisent une **AABB monde** dérivée de cette zone (pas le mesh brut).

---

## 3. Propriétés principales (catégories Unreal)

### `Casino | Placement | Exclusion zone`

| Propriété | Description |
|-----------|-------------|
| **PlacementExclusionHalfExtent** | Demi-tailles **locales** de la zone interdite aux autres objets (X, Y, Z fin pour un « tapis » au sol). |
| **PlacementExclusionCenterOffset** | Centre de cette zone par rapport à l’**origine de l’acteur** (permet le décalage asymétrique). |
| **bShowExclusionZonePreview** | Affiche le volume d’aperçu (souvent `true` en dev ou pendant le drag). |
| **ExclusionZonePreviewMaterial** | Matériau **translucide** (ex. vert) pour le volume d’aperçu. |
| **ExclusionZonePreviewMesh** | Optionnel : remplace le cube moteur par un plan / autre mesh. |

### Composant `ExclusionZoneVisualizer`

Mesh non collisionné, mis à l’échelle pour refléter la zone. Exposé en lecture pour matériaux / tri en BP.

### `Casino | Placement | Debug`

| Propriété | Description |
|-----------|-------------|
| **bShowFootprintDebug** | Dessine la boîte en debug (fil de fer, désactivé en shipping). |
| **FootprintDebugColor** | Couleur du debug. |

### `Casino | Highlight`

| Propriété | Description |
|-----------|-------------|
| **HighlightStencilValue** | Stencil Custom Depth pour le post-process « contour ». |

### `Casino | NPC`

| Propriété | Description |
|-----------|-------------|
| **NPCInteractionSlots** | Tableau de **`FTheHouseNPCInteractionSlot`** : `SocketName` + `PurposeTag` (ex. `Sit`, `Deal`) pour attacher le PNJ et choisir la logique / montage côté Blueprint. |

---

## 4. API utile (Blueprint / C++)

| Fonction | Usage |
|----------|--------|
| **GetLocalPlacementExclusionBox** | Boîte locale (offset + demi-tailles). |
| **GetWorldPlacementExclusionBox** | Boîte monde pour la pose actuelle. |
| **TestFootprintOverlapsOthersAt(WorldTransform, IgnoreActor)** | `true` si la zone chevauche un autre `ATheHouseObject` — à utiliser **pendant** le drag avant de valider. |
| **RefreshExclusionZonePreviewMesh** | Recalcule position / scale du visualiseur après changement de tailles. |
| **SetExclusionZonePreviewVisible(bool)** | Affiche ou cache le volume vert (ex. `true` au drag, `false` une fois posé). |
| **SetHighlighted(bool)** | Active Custom Depth sur les primitives (sauf le visualiseur d’exclusion). |

*Les anciennes fonctions **GetLocalFootprintBox** / **GetWorldFootprintBox** pointent vers la même logique (marquées deprecated au profit des noms « PlacementExclusion »).*

---

## 5. Workflow recommandé

### Blueprint d’un objet (table, machine, …)

1. Parent de classe : **`TheHouseObject`** (ou sous-classe C++).
2. Ajouter meshes / collision gameplay comme d’habitude.
3. Régler **PlacementExclusionHalfExtent** et **PlacementExclusionCenterOffset** pour correspondre au « vert ».
4. Créer un **Material Instance** translucide et l’assigner à **ExclusionZonePreviewMaterial**.
5. Remplir **NPCInteractionSlots** (sockets présents sur le skeletal mesh importé).

### Drag & drop sur la carte

1. Pendant le mouvement : suivre le sol / grille, construire un **FTransform** cible.
2. Appeler **TestFootprintOverlapsOthersAt** ; si `true`, refuser la pose ou snapper ailleurs.
3. **SetExclusionZonePreviewVisible(true)** au début du drag, **false** au lâcher si tu ne veux plus voir la zone.

### Contour (outline)

1. Meshes : **Render Custom Depth** activé si besoin.
2. Projet : **Custom Depth-Stencil** (souvent avec stencil).
3. Post-process / matériau global qui lit le stencil (tutoriels UE « outline custom depth »).

---

## 6. Checklist de reprise (quand tu reviens sur le projet)

- [ ] Lire [Docs/README.md](../../README.md) et ce fichier.
- [ ] Consulter [Changelog](../../Changelog/CHANGELOG.md) pour les derniers changements.
- [ ] Ouvrir `TheHouseObject.h` pour les noms exacts des `UPROPERTY`.
- [ ] Vérifier qu’un **matériau d’aperçu** est assigné sur les BP d’objets (sinon le volume peut rester gris / invisible selon le mesh).
- [ ] Brancher **SetExclusionZonePreviewVisible** dans la logique de placement (PlayerController ou mode « construction »).
- [ ] Tester **TestFootprintOverlapsOthersAt** avec deux objets proches pour valider tailles et offset.

---

## 7. Limitations connues

- L’overlap est basé sur **boîtes alignées aux axes** après transformation (approximation ; suffisant pour des « emplacements » de meubles).
- Le visualiseur utilise un **cube** par défaut (échelle non uniforme) ; un plan peut mieux coller au sol si tu assignes **ExclusionZonePreviewMesh**.

---

*Voir les entrées associées dans [CHANGELOG.md](../../Changelog/CHANGELOG.md).*
