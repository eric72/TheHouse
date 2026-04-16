# Documentation TheHouse

Vue d’ensemble du dossier **`Docs/`** : une arborescence par **fonctionnalité**, plus un **journal des mises à jour**.

---

## Guide développeur (code C++)

| Ressource | Lien |
|-----------|------|
| **Index** | [Developer/README.md](Developer/README.md) |
| **Français (Markdown)** | [Developer/GUIDE_FR.md](Developer/GUIDE_FR.md) |
| **English (Markdown)** | [Developer/GUIDE_EN.md](Developer/GUIDE_EN.md) |
| **Site web statique** | Dans [`site/`](site/) : `npm install` puis `npm run dev` (Vite, port 5173). Sinon ouvrir [`site/index.html`](site/index.html) directement. |

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
├── Developer/                ← guide C++ (FR/EN .md + README)
├── site/                     ← site statique (index.html, fr/, en/)
├── Features/
│   └── <NomFeature>/
│       └── README.md
└── Changelog/
    └── CHANGELOG.md
```
