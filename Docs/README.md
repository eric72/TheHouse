# Documentation TheHouse

Vue d’ensemble du dossier **`Docs/`** : une arborescence par **fonctionnalité**, plus un **journal des mises à jour**.

## Documentation site web

Une **version site web** de la documentation (pages HTML, navigation latérale, sélecteur de langue FR/EN) vit dans **`site/`** : voir [site/index.html](site/index.html) ou, pour un serveur local avec rechargement, `npm install` puis `npm run dev` depuis **`Docs/site/`** (Vite). Le contenu reprend la structure des guides développeur et des pages d’aperçu.

---

## Guide développeur (code C++)

| Ressource | Lien |
|-----------|------|
| **Index** | [Developer/README.md](Developer/README.md) |
| **Français (Markdown)** | [Developer/GUIDE_FR.md](Developer/GUIDE_FR.md) |
| **English (Markdown)** | [Developer/GUIDE_EN.md](Developer/GUIDE_EN.md) |
| **Documentation site web** | Dans [`site/`](site/) : `npm install` puis `npm run dev` (Vite, port 5173). Sinon ouvrir [`site/index.html`](site/index.html) dans le navigateur. |

---

## Fonctionnalités (`Features/`)

Chaque sous-dossier décrit une feature (code, BP, workflow). L’**[index Features](Features/README.md)** liste tout et indique quelles parties du `Source/` vont où.

| Dossier | Sujet |
|---------|--------|
| [Features/CasinoPlaceableObjects](Features/CasinoPlaceableObjects/README.md) | Objets casino, `ATheHouseObject`, zone d’exclusion, slots, placement |
| [Features/SmartWall](Features/SmartWall/README.md) | `ATheHouseSmartWall`, occlusion, matériau, Fill Cap |
| [Features/RTS_UI](Features/RTS_UI/README.md) | Panneau RTS, menus contextuels, ordres PNJ, paramètres objet |
| [Features/Localization](Features/Localization/README.md) | Sous-système langue, SaveGame, scripts `RunLocalizationStep.ps1` |
| [Features/NPCWorld](Features/NPCWorld/README.md) | Subsystem, archetypes, volume d’éjection, IA slot |

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
├── PROJECT_OVERVIEW.md       ← vision courte + titres de travail
├── SYSTEMS.md                ← systèmes (aligné code)
├── Developer/                ← guide C++ (FR/EN .md + README)
├── site/                     ← site statique (index.html, fr/, en/)
├── Features/
│   ├── README.md             ← index de toutes les features
│   ├── CasinoPlaceableObjects/
│   ├── SmartWall/
│   ├── RTS_UI/
│   ├── Localization/
│   └── NPCWorld/
└── Changelog/
    └── CHANGELOG.md
```
