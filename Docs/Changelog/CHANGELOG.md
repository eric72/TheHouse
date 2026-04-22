# Changelog — TheHouse (documentation & features suivies)

Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/). Les entrées reflètent surtout ce qui impacte le **gameplay**, la **doc** ou les **API** exposées aux Blueprints.

---

## [Non versionné] — en cours

### Documentation

- **Passage à jour global :** `GUIDE_EN.md` aligné avec les menus PNJ / ordres (FR) ; arborescence FR/EN enrichie (`Localization/`, `UI/`, `NPCEjectRegionVolume`, `BP_SmartWall`) ; `Docs/PROJECT_OVERVIEW.md` et `Docs/SYSTEMS.md` réécrits ; nouveaux index **`Docs/Features/RTS_UI`**, **`Localization`**, **`NPCWorld`** + table de couverture `Source/` dans `Docs/Features/README.md` ; site `Docs/site/fr|en/index.html` (chemins scripts portables, table arborescence, références) ; exemples localisation sans chemin machine dans les guides.
- **PNJ / NPC:** architecture documentée dans `Docs/Developer/GUIDE_FR.md` et `Docs/Developer/GUIDE_EN.md` ; archetypes refactorés en **`NPC/Archetypes/`** (`UTheHouseNPCArchetype` racine + Staff / Customer / Special ; racine non abstraite pour le picker d’assets), **`StaffMonthlySalary`** runtime sur le personnage ; même résumé sur `Docs/site/`.
- **Localisation :** `Scripts/RunLocalizationStep.ps1` (toutes les étapes dashboard : Gather, Compile, Export/Import PO, dialogue script CSV, import dialogue WAV, rapports) ; guides `GUIDE_FR.md` / `GUIDE_EN.md` et pages `Docs/site/` mises à jour ; `GatherLocalization.ps1` / `CompileLocalization.ps1` délèguent au script maître.
- **RTS UI :** section *panneau principal (`RTS Main Widget Class`) vs menu contextuel (`RTS Context Menu Widget Class`)* dans `Docs/Developer/GUIDE_FR.md` et `Docs/Developer/GUIDE_EN.md` (table des matières mise à jour).
- **RTS catalogue / stock :** champ optionnel **`Thumbnail`** sur `FTheHousePlaceableCatalogEntry` + affichage vignette dans les listes du widget RTS principal (`TheHouseRTSMainWidget.cpp`).
- Guide développeur C++ bilingue : `Docs/Developer/GUIDE_FR.md`, `Docs/Developer/GUIDE_EN.md`, index `Docs/Developer/README.md` ; site statique `Docs/site/` (navigation + sélecteur de langue en bas à droite).
- Structure `Docs/` : `Features/<Nom>/README.md` par fonctionnalité, `Changelog/CHANGELOG.md` pour les mises à jour.
- Déplacement de la doc « objets casino » vers `Docs/Features/CasinoPlaceableObjects/README.md`.
- Ajout de la doc **SmartWall** : `Docs/Features/SmartWall/README.md` (catégories BP, occlusion, `MinVisibleHeightCm`, matériau, Fill Cap expérimental, mesh pivot `SM_WallPivotBottomLeft.obj`).

---

## Historique (à compléter)

Quand tu tagues une version ou un jalon, ajoute une section du type :

```markdown
## [0.1.0] — YYYY-MM-DD

### Ajouté
- …

### Modifié
- …

### Corrigé
- …
```

---

## Index rapide des features documentées

| Feature | Doc |
|---------|-----|
| Objets casino / placement | [Features/CasinoPlaceableObjects/README.md](../Features/CasinoPlaceableObjects/README.md) |
| SmartWall | [Features/SmartWall/README.md](../Features/SmartWall/README.md) |
| RTS / menus UMG | [Features/RTS_UI/README.md](../Features/RTS_UI/README.md) |
| Localisation | [Features/Localization/README.md](../Features/Localization/README.md) |
| PNJ & monde | [Features/NPCWorld/README.md](../Features/NPCWorld/README.md) |
| Index global | [Features/README.md](../Features/README.md) |
