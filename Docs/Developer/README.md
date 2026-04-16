# Documentation développeur / Developer documentation

| Langue | Markdown | Site web |
|--------|----------|----------|
| **Français** | [GUIDE_FR.md](GUIDE_FR.md) | Site : `Docs/site/` → `npm install` puis `npm run dev` (Vite), ou ouvrir [`index.html`](../site/index.html) |
| **English** | [GUIDE_EN.md](GUIDE_EN.md) | Same: Vite dev server or open [`index.html`](../site/index.html) |

Les fichiers **`.md`** dans ce dossier sont la référence complète (tables, pièges, configuration). Le **site** dans `Docs/site/` reprend la même structure avec navigation latérale et un sélecteur de langue. Il n’y a **pas** de React/Next : uniquement HTML/CSS ; **Vite** sert seulement de serveur local (`npm run dev`).

The **`.md`** files here are the full reference. The **`Docs/site/`** pages are plain HTML/CSS — **no React/Next**. **Vite** is only a local dev server.

---

## Aperçu rapide / Quick overview

- **Moteur / Engine:** Unreal **5.7**, module C++ **`TheHouse`**
- **Vision / Vision:** RTS casino management + FPS secondaire (sans gestion casino en FPS)
- **Fichiers clés / Key files:** `TheHousePlayerController`, `TheHouseObject`, `TheHouseGameModeBase`, `TheHouseGameViewportClient`

Voir aussi `../README.md` (index `Docs/`) pour les features détaillées (SmartWall, objets plaçables, changelog).

See also `../README.md` for feature docs (SmartWall, placeable objects, changelog).
