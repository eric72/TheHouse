# 🧱 Tutoriel : Créer le Mur Intelligent ("Smart Wall")

Ce guide explique comment configurer le matériau et le blueprint pour le système de transparence "Sims-style".

---

## 🎨 Partie 1 : Le Matériau (`M_SmartWall`)

Ce shader gère la transparence et l'apparence du mur.

### 1. Paramètres de base

Dans les **Details** du Material :

- **Blend Mode** : `Masked` (Optimisé, coupe nette).
- **Two Sided** : `Enabled` (Coché) pour voir l'intérieur.
- **DitherTemporalAA** : À utiliser pour adoucir le masque (effet de fondu premium).

### 2. Le Graphique (Nodes)

**Logique de Coupe (Transparence) :**

1.  **Scalar Parameter** : Nommez-le `CameraFade` (0.0 = Visible, 1.0 = Caché).
2.  **Opacity Mask** :
    - Calcul : `(WorldPosition.Z - ObjectPosition.Z - 50.0)`
    - Multiplié par `CameraFade`.
    - Envoyé dans un **DitherTemporalAA** pour le fondu.
    - Connecté à **Opacity Mask**.

**Logique d'Apparence (Double Face) :**

1.  **Texture** : Convertissez votre Texture Sample en Paramètre (`MainTexture`).
2.  **Couleur Intérieure** : Créez une couleur sombre (Gris Foncé).
3.  **Choix** : Utilisez un nœud **If** avec **TwoSidedSign** pour afficher :
    - Face Avant (>0) : La Texture `MainTexture`.
    - Face Arrière (<0) : La Couleur Béton.
    - Connecté à **Base Color**.

---

## 🏗️ Partie 2 : Le Blueprint (`BP_SmartWall`)

Ce blueprint permet de placer facilement des murs configurables.

### 1. Composants

- **RootScene (Scene Component)** : La racine invisible. Sert de **Pivot** (0,0,0).
- **WallMesh (Static Mesh)** : Le mur visible.
  - Enfant de `RootScene`.
  - **Décalage (Translation)** :
    - `Z = +150` (pour le poser au sol si hauteur 300).
    - `X = +200` (pour mettre le pivot dans le coin).
  - **Collision** : `Collision Presets` -> Dépliez -> **Camera : Ignore**.

### 2. Construction Script (Personnalisation)

Pour changer la texture de chaque mur individuellement :

1.  **Create Dynamic Material Instance** (Target: WallMesh, Source: M_SmartWall).
2.  **Set Texture Parameter Value** (Param Name: `MainTexture`).
3.  **Promote to Variable** : Créez une variable `WallTexture` (Oeil ouvert / Instance Editable).

---

## ✅ Utilisation

1.  Placez `BP_SmartWall` dans le niveau.
2.  Sélectionnez-le.
3.  Dans **Details**, changez la texture "Wall Texture".
4.  Lancez le jeu : le mur se coupe quand la caméra passe derrière !
