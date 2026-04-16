# Changelog — TheHouse (documentation & features suivies)

Format inspiré de [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/). Les entrées reflètent surtout ce qui impacte le **gameplay**, la **doc** ou les **API** exposées aux Blueprints.

---

## [Non versionné] — en cours

### Documentation

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
