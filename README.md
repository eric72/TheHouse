# TheHouse

Projet **Unreal Engine 5.7** (module C++ `TheHouse`). Titre de travail côté design : **The House**. Autres idées de noms notées pour le projet : **Sum Zero**, **Zero Sum**.

Ce fichier décrit la **prise en main** après clone du dépôt : prérequis, ouverture du projet, et configuration **Cursor / Visual Studio Code** pour l’IntelliSense C++.

Pour l’architecture du jeu, les systèmes (PNJ, objets plaçables, etc.) et la doc détaillée : **[Docs/Developer/README.md](Docs/Developer/README.md)** (guides FR/EN en Markdown) et le **site de documentation** dans **[Docs/site/](Docs/site/)** (voir section 4).

---

## Prérequis

| Outil | Détail |
|--------|--------|
| **Unreal Engine** | Version alignée sur [`TheHouse.uproject`](TheHouse.uproject) : champ **`EngineAssociation`** (actuellement **5.7**). Installation via Epic Games Launcher, en général sous `C:\Program Files\Epic Games\UE_5.7`. |
| **Toolchain C++ (Windows)** | **Visual Studio 2022** ou **Build Tools** avec charge de travail **« Développement de jeu avec C++ »** / composants MSVC, Windows SDK (comme pour tout projet UE natif). |
| **Git** | Pour cloner le dépôt. |
| **IDE (optionnel mais recommandé)** | **Cursor** ou **Visual Studio Code** + extension **C/C++** (`ms-vscode.cpptools`) pour éditer le C++ avec complétion et navigation. |

Si le moteur n’est pas dans le chemin Epic par défaut, définissez une variable d’environnement utilisateur pointant vers la racine du moteur (le dossier qui **contient** `Engine\`, pas `Engine` lui-même) :

- `UE_ENGINE_ROOT` ou `UNREAL_ENGINE_ROOT`

---

## 1. Cloner et ouvrir le projet dans l’éditeur Unreal

1. Clonez le dépôt puis placez-vous dans le dossier du projet (là où se trouve **`TheHouse.uproject`**).
2. Double-cliquez sur **`TheHouse.uproject`**, ou ouvrez-le via le **Epic Games Launcher** / **Bibliothèque**.
3. Si une association de version moteur est demandée, choisissez **UE 5.7** (ou régénérez les binaires du projet depuis l’éditeur / les sources).

La **première compilation** des modules C++ peut être lancée depuis l’éditeur (« Compiler »), ou avec **Visual Studio** si vous générez la solution (voir ci-dessous).

---

## 2. Visual Studio — générer la solution (optionnel)

Pour générer **`TheHouse.sln`** et compiler comme d’habitude sous Windows avec Unreal :

- Clic droit sur **`TheHouse.uproject`** → **Generate Visual Studio project files**  
  *ou*
- En ligne de commande (adapter le chemin du moteur et du `.uproject`) :

```powershell
dotnet "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.dll" `
  -projectfiles `
  "-project=C:\chemin\vers\TheHouse\TheHouse.uproject" `
  -game -engine
```

*(Sans `-vscode`, UBT utilise le format d’IDE par défaut — en général Visual Studio. Si besoin uniquement des fichiers VS Code / Cursor, utilisez la section 3.)*

---

## 3. Cursor / VS Code — IntelliSense C++

Les fichiers d’indexation C++ (**`compileCommands_*.json`**, **`c_cpp_properties.json`**, etc.) sont **générés localement** et contiennent des chemins absolus (compilateur, moteur, projet). **Ils ne sont en principe pas versionnés** ; chaque machine doit les régénérer après clone.

### Extensions

À l’ouverture du dossier du projet, l’éditeur peut proposer les **extensions recommandées** (fichier [`.vscode/extensions.json`](.vscode/extensions.json)) : au minimum **C/C++** (Microsoft).

### Régénérer l’IntelliSense (méthode recommandée)

Dans Cursor / VS Code :

1. **Terminal** → exécuter la tâche **`UE: Refresh VS Code / Cursor IntelliSense`**  
   *(menu **Run Task…** / `Ctrl+Shift+B` selon configuration),*  
   **ou**
2. Lancer le script PowerShell à la racine du projet :

```powershell
powershell -ExecutionPolicy Bypass -File "Scripts\RefreshVSCodeIntellisense.ps1"
```

Le script lit **`EngineAssociation`** dans `TheHouse.uproject`, cherche le moteur sous le chemin Epic habituel ou via `UE_ENGINE_ROOT` / `UNREAL_ENGINE_ROOT`, puis appelle **UnrealBuildTool** avec `-projectfiles -vscode`.

### Après la régénération

Dans la palette de commandes (`Ctrl+Shift+P`) :

1. **C/C++: Reset IntelliSense Database**
2. **Developer: Reload Window**

Ensuite l’autocomplétion et la navigation (**F12**, etc.) devraient refléter les mêmes définitions que le build du module **`TheHouse`**.

### À refaire quand ?

Relancez la tâche ou le script après des changements structurels : nouveau dossier de sources dans le module, modification importante de **`TheHouse.Build.cs`**, nouveau plugin C++, ou si l’IDE « perd » les symboles.

---

## 4. Documentation (Markdown + site web)

En plus des **guides longs** en `.md`, le dépôt inclut une **documentation site web** statique (HTML/CSS, navigation, FR/EN) dans **`Docs/site/`**.

| Ressource | Contenu |
|-----------|---------|
| [Docs/README.md](Docs/README.md) | Index de tout le dossier `Docs/` (features, changelog, lien vers le site) |
| [Docs/Features/README.md](Docs/Features/README.md) | Index des features (casino, SmartWall, RTS UI, localisation, PNJ monde) |
| [Docs/Developer/README.md](Docs/Developer/README.md) | Index développeur (FR/EN) + lien vers le site |
| [Docs/Developer/GUIDE_FR.md](Docs/Developer/GUIDE_FR.md) | Guide long (architecture, localisation, pièges) |
| [Docs/Developer/GUIDE_EN.md](Docs/Developer/GUIDE_EN.md) | Same in English |
| **Site web** | [Docs/site/](Docs/site/) : `npm install` puis `npm run dev` dans `Docs/site/` (Vite, port 5173), ou ouvrir [Docs/site/index.html](Docs/site/index.html) dans le navigateur |

---

## English (short)

- **Working title ideas:** **The House** (current direction), also **Sum Zero**, **Zero Sum**.
- **Engine:** UE **5.7** (see `EngineAssociation` in `TheHouse.uproject`). Install via Epic Launcher; optional env vars **`UE_ENGINE_ROOT`** or **`UNREAL_ENGINE_ROOT`** if the engine is not under `Program Files\Epic Games\UE_5.7`.
- **C++ workload:** MSVC + Windows SDK (standard Unreal Windows setup).
- **Cursor / VS Code:** After clone, run task **`UE: Refresh VS Code / Cursor IntelliSense`** or `Scripts\RefreshVSCodeIntellisense.ps1`, then **C/C++: Reset IntelliSense Database** and **Developer: Reload Window**.
- **Docs:** Markdown guides under [Docs/Developer/](Docs/Developer/) and a **static doc website** under [Docs/site/](Docs/site/) (`npm run dev` in `Docs/site/` or open `index.html`).
