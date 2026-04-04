# Documentation TheHouse

Vue d’ensemble du dossier **`Docs/`** : une arborescence par **fonctionnalité**, plus un **journal des mises à jour**.

---

## Fonctionnalités (`Features/`)

Chaque sous-dossier décrit une feature (code, BP, workflow). Ouvre le **`README.md`** du dossier concerné.

| Dossier | Sujet |
|---------|--------|
| [Features/CasinoPlaceableObjects](Features/CasinoPlaceableObjects/README.md) | Objets casino, `ATheHouseObject`, zone d’exclusion, PNJ, placement |
| [Features/SmartWall](Features/SmartWall/README.md) | `ATheHouseSmartWall`, occlusion, hauteur minimale, matériau, Fill Cap |

*Ajoute un dossier par nouvelle grosse feature (ex. `Features/RTSCamera`).*

---

## Mises à jour (`Changelog/`)

| Fichier | Rôle |
|---------|------|
| [Changelog/CHANGELOG.md](Changelog/CHANGELOG.md) | Journal des changements notables (versions, refactors doc, breaking changes). |

---

## Arborescence type

```
Docs/
├── README.md                 ← tu es ici
├── Features/
│   └── <NomFeature>/
│       └── README.md
└── Changelog/
    └── CHANGELOG.md
```
