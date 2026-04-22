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
- **Fichiers clés / Key files:** `TheHousePlayerController`, `TheHouseObject`, `TheHouseGameModeBase`, `TheHouseGameViewportClient`, socle PNJ (`ATheHouseNPCCharacter`, `UTheHouseNPCSubsystem`, `UTheHouseNPCArchetype`)

**Système PNJ / NPC system:** [GUIDE_FR § Système PNJ](GUIDE_FR.md#système-pnj-architecture) · [GUIDE_EN § NPC system](GUIDE_EN.md#npc-system-architecture)

Voir aussi `../README.md` (index `Docs/`) pour les features détaillées (SmartWall, objets plaçables, changelog).

**Localisation (ligne de commande) :** `Scripts/RunLocalizationStep.ps1` (`-Step Gather|Compile|Export|Import|…`) ; raccourcis `GatherLocalization.ps1`, `CompileLocalization.ps1`. Détail : [GUIDE_FR.md § Localisation](GUIDE_FR.md#localisation-pipeline-complet-en-ligne-de-commande) / [GUIDE_EN.md § Localization](GUIDE_EN.md#localization-full-command-line-pipeline).

See also `../README.md` for feature docs (SmartWall, placeable objects, changelog).

**Localization (CLI):** `Scripts/RunLocalizationStep.ps1` with `-Step Gather|Compile|Export|Import|…`; wrappers `GatherLocalization.ps1`, `CompileLocalization.ps1`. Details: [GUIDE_EN.md § Localization](GUIDE_EN.md#localization-full-command-line-pipeline).
