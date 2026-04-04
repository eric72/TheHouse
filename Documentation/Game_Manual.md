# Manuel de Jeu & Développement - "The House"

Documentation complète des contrôles et des systèmes implémentés.

---

## 🎮 Contrôles du Jeu

### Mode RTS (Vue du dessus)

C'est le mode principal pour la construction et la gestion.

| Action                | Touche(s)                                  | Notes                                         |
| :-------------------- | :----------------------------------------- | :-------------------------------------------- |
| **Bouger la Caméra**  | `Z`, `Q`, `S`, `D` (ou `W`, `A`, `S`, `D`) | Déplacement relatif à la vue.                 |
| **Zoomer**            | `Molette Souris`                           | Le zoom se fait vers le curseur de la souris. |
| **Tourner la Caméra** | `Alt` + `Souris (Bouger)`                  | Ou modifier pour Clic Droit selon préférence. |
| **Passer en FPS**     | `F`                                        | Prend possession du personnage au sol.        |

### Mode FPS (Vue 1ère personne)

Pour visiter la maison "à hauteur d'homme".

| Action            | Touche(s)          | Notes                          |
| :---------------- | :----------------- | :----------------------------- |
| **Marcher**       | `Z`, `Q`, `S`, `D` | Contrôles classiques de FPS.   |
| **Regarder**      | `Souris`           |                                |
| **Sauter**        | `Espace`           | (Si activé)                    |
| **Passer en RTS** | `F`                | La caméra repart dans le ciel. |

---

## 🛠️ Systèmes Techniques

### 1. Système de Mur Intelligent (Smart Wall)

Les murs qui bloquent la vue de la caméra disparaissent automatiquement.

- **Fonctionnement** : Le PlayerCameraPawn lance un rayon vers le sol. S'il touche un acteur avec le Tag `SeeThroughWall`, il active la transparence.
- **Visuel** : Utilise le shader `M_SmartWall` pour un effet de coupe + fondu "Dithered".
- **Pivot** : Les murs ont un pivot décalé (au coin) pour faciliter l'alignement type grille.

### 2. Caméra Hybride

Le jeu utilise deux Pions (Pawns) distincts :

- `ATheHouseCameraPawn` : Flotte, pas de collision, gère le SpringArm et la détection d'occlusion.
- `ATheHouseFPSCharacter` : Personnage physique standard (Character Movement).
- `ATheHousePlayerController` : Le cerveau qui possède l'un ou l'autre et synchronise leur position lors du changement (`SwitchToRTS` / `SwitchToFPS`).

---

## 📁 Structure du Projet

- **Source/TheHouse/Player** : Contient tous les fichiers liés au joueur (Controller, CameraPawn, SmartWall).
- **Documentation/** : Ce dossier contient les guides.
