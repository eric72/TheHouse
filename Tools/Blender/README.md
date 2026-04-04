# Blender — génération d’assets

## Table de blackjack

Script : `generate_blackjack_table.py`  
Sortie dans `export/` :
- `BlackjackTable.fbx` — géométrie  
- `BlackjackTable_BaseColor.png` — couleur (bois + feutre vert, bake sur les UV)  
- `BlackjackTable_Normal.png` — normal map (tangent space)

### Exécution (PowerShell)

```powershell
& "C:\Program Files\Blender Foundation\Blender 5.0\blender.exe" --background --python "c:\Users\Eric\Documents\Unreal Projects\TheHouse\Tools\Blender\generate_blackjack_table.py"
```

---

## Échelle Blender → Unreal (corrigé dans le script)

Le mesh est modélisé **directement en centimètres** dans Blender (220 × 110 cm, etc.), comme dans Unreal (**1 uu = 1 cm**). L’export FBX utilise **`global_scale=1.0`** : tu peux mettre **Uniform Scale = `1.0`** dans la fenêtre Interchange au réimport (plus besoin de **`0,01`** si tu régénères le FBX avec le script actuel).

Si tu avais déjà importé une ancienne version avec **0,01**, repasse à **`1.0`** après avoir relancé le script et réimporté.

---

## Unreal Engine — taille cohérente avec le personnage (FPS)

Une table de blackjack réelle fait environ **220 cm × 110 cm**, plateau **~76 cm** du sol.

### Si la table est énorme (taille « maison »)

Cause fréquente : **double échelle** (FBX + réglage Interchange / ancien export `global_scale=100`).

---

### Je ne trouve pas les options d’import (UE 5, interface en français)

Les noms changent selon la version ; essaie **dans cet ordre** :

#### 1) Depuis le Content Browser (souvent le plus simple)

1. **Un seul clic** sur ton **Static Mesh** (ne double-clique pas : ça ouvre l’éditeur).
2. Regarde le panneau **Détails** à **droite** (pas en bas).
3. Fais défiler jusqu’à une section du type :
   - **Données d’importation d’asset** / **Asset Import Data**
   - ou **Informations** / **Fichier source**
4. Déplie-la : tu dois voir le **chemin du fichier** et souvent un bouton **Réimporter** ou **Modifier** / **…** / **crayon** pour rouvrir la fenêtre **FBX Import Options** (avec **Transform** et **Échelle d’importation uniforme**).

Si le panneau **Détails** n’est pas visible : menu **Fenêtre** → **Détails** (ou **Window** → **Details**).

#### 2) Depuis l’éditeur de Static Mesh (double-clic sur l’asset)

1. **Double-clic** sur le Static Mesh pour ouvrir la fenêtre du mesh.
2. Cherche un onglet ou un panneau nommé **Détails de l’asset**, **Asset Details**, **Informations sur l’asset** (souvent **en bas** de la fenêtre).
3. Même section **Données d’importation d’asset** → bouton pour **éditer** les paramètres d’import.

Si tu ne vois que le viewport : menu **Fenêtre** → **Détails de l’asset** / **Asset Details** (ou fais glisser l’onglet depuis **Fenêtre**).

#### 3) Toujours rien ?

- Utilise la **recherche** du panneau Détails : clique dans **Détails** → petite **loupe** ou champ de recherche → tape **`import`** ou **`échelle`**.
- **Solution de secours** : supprime le mesh et **réimporte** le FBX par glisser-déposer (méthode **B** ci‑dessous) : la grande fenêtre **Importer** affiche toujours **Transform**.
- **Sans toucher à l’import** : règle le **Scale** du **Static Mesh** sur ton Blueprint (**0,01** si la table est ~100× trop grande).

---

### « Données d’importation d’asset » n’existe pas dans mon panneau Détails

C’est **possible** selon ta version d’UE 5 et le **pipeline d’import** (souvent **Interchange** pour le FBX) : la section n’a **pas le même nom** ou n’est **pas affichée** quand tu regardes les mauvais éléments.

**Vérifie d’abord :**

1. Tu as bien fait **un clic** sur l’**asset** Static Mesh dans le **Content Browser** (icône bleue du mesh), **pas** sur une **instance** de la table placée dans le niveau.
2. En haut du panneau **Détails**, désactive **Afficher uniquement les propriétés modifiées** / *Show Only Modified* (si c’est activé, des catégories entières disparaissent).
3. Cherche d’autres libellés : **Fichier source**, **Source File**, **Chemin**, **Interchange**, **Importer**, ou une ligne avec le **.fbx** cliquable.

**Si la section n’existe vraiment pas** : ce n’est pas bloquant. Beaucoup de projets UE 5 gèrent l’échelle ainsi :

| Objectif | Action |
|----------|--------|
| Voir la fenêtre **Transform** / **Échelle** | **Supprime** le Static Mesh dans Content, puis **réimporte** le FBX (glisser-déposer) : la fenêtre **Importer** / **Interchange** s’ouvre avec les options. |
| Corriger vite la taille | **Blueprint** → composant **Static Mesh** → **Transform** → **Scale** (ex. **0,01** si ~100× trop grand). |

Tu n’es **pas obligé** d’avoir « Données d’importation d’asset » pour jouer : le **Scale** sur le composant ou un **nouvel import** du fichier suffit.

---

### Unreal en français : pourquoi « Réimporter » ne montre pas Transform

C’est **normal** : un **Réimporter** (clic droit sur le Static Mesh) réutilise les **anciens paramètres**. La fenêtre avec **Transform / Échelle** n’apparaît que pour un **nouvel import** ou si tu **ouvres les options d’import** depuis l’asset.

#### A) Ouvrir les paramètres d’import (sans supprimer l’asset)

1. **Double-clic** sur le **Static Mesh** dans le Content Browser (éditeur de mesh).
2. En bas : panneau **Détails de l’asset** / **Asset Details** (pas le viewport).
3. Cherche **Données d’importation d’asset** (*Asset Import Data*) ou **Informations sur l’import**.
4. Utilise **Réimporter…**, **Modifier les paramètres d’import**, ou l’icône **crayon / roue dentée** près du fichier source — selon UE 5.x, cela rouvre **FBX Import Options** avec **Transform**.
5. Section **Transform** (ou **Transformation**) → **Échelle d’importation uniforme** (*Import Uniform Scale*) = **`1.0`** (pas `100` en plus du script Blender).

#### B) Forcer la fenêtre complète d’import

1. **Supprime** le Static Mesh dans le Content Browser (ou déplace-le).
2. **Glisse-dépose** à nouveau `BlackjackTable.fbx` dans le dossier Content (ou **Importer**).

→ La fenêtre **Importer** s’affiche avec **Transform** / **Échelle d’importation uniforme**.

#### C) Corriger comme le « cube de base » (scale sur le Blueprint)

Le **cube par défaut** fait souvent **100 unités de côté** (**1 m**). Pour aligner vite ta table **sans** retrouver Transform :

1. Ouvre le **Blueprint** de la table → composant **Static Mesh**.
2. **Détails** → **Transform** → **Scale** :
   - table **~100× trop grande** → **`0.01`** sur X, Y, Z ;
   - **~10× trop grande** → **`0.1`**, etc.

C’est un correctif **sur l’acteur** ; l’asset peut rester énorme dans le navigateur, mais **en jeu** la taille sera cohérente avec le personnage FPS.

**Repère** : surface du tapis vers **70–80 cm** au sol ; compare visuellement avec un **cube 1 m** placé à côté.

---

## Couleur toujours moche — checklist (dans l’ordre)

1. **Nanite** : ouvre le **Static Mesh** → **Détails** → **Paramètres Nanite** → **désactive Nanite** sur ce mesh (~370 tris, pas utile). C’est la cause n°1 des verts / oranges « dégueulasses ».
2. **Viewport** (éditeur mesh **et** niveau) : mode **Lit** / **Éclairé**, pas **Unlit** ni **Buffer Visualization**. Coupe toute option **Nanite visualization** / **Visualisation Nanite** dans la barre du viewport.
3. **Bon composant** : sur **BlackJackTable_BP**, le matériau `M_BlackjackTable` doit être sur le composant **`BlackjackTable`** (ton mesh), **pas** sur **ExclusionZoneVisualizer** (vert de debug).
4. **Texture BaseColor** : ouvre `BlackjackTable_BaseColor` → **sRGB** doit être **coché** (couleur). Si tu l’as importée comme normal map par erreur, les couleurs seront fausses.
5. **Texture Normal** : **sRGB décoché**, compression **Normal map**.
6. **Slot** : **Materials → Element 0** = ton matériau (pas « aucun », pas le défaut gris du moteur si tu veux la texture).
7. **Tester en jeu** (**Alt+P**) : si c’est bon en jeu mais moche dans l’éditeur mesh, c’était surtout Nanite / vue debug.

---

## Pourquoi la table est verte / orange « dégueulasse » dans l’éditeur de mesh

Souvent ce n’est **pas** ta texture : c’est la **visualisation Nanite** (clusters colorés pour le debug).

- Désactive **Nanite** sur l’asset (voir checklist ci‑dessus).
- Viewport en **Lit**, sans overlay Nanite.

## Pourquoi il n’y a « pas de texture » sur le mesh

Le script produit **`BlackjackTable_BaseColor.png`** (bois + feutre bake sur les UV) et **`BlackjackTable_Normal.png`**. Le FBX est exporté **sans matériau** : dans Unreal, crée **`M_BlackjackTable`** en branchant ces deux textures (voir ci‑dessous). Sans ce matériau, tu vois le défaut du moteur ou le mode debug Nanite ci‑dessus.

---

## Textures et normal map (le mesh n’est pas « texturé » tout seul)

Le **jaune / orange** dans la vue = souvent **matériau par défaut** ou slot vide. La **normal map** ne change **pas** la couleur : elle ajoute du relief à la lumière. Il faut un **Material** (ou **Material Instance**) assigné au Static Mesh.

### 1. Importer les textures

1. Glisse `BlackjackTable_BaseColor.png` et `BlackjackTable_Normal.png` dans `Content/…`.
2. Pour la normal : ouvre la texture → **Compression** → mode adapté aux normales (**Normal Map** / BC5 selon version).
3. La **BaseColor** reste en **sRGB** / **Default** (couleur).

### 2. Créer un matériau

1. Clic droit → **Material** (ex. `M_BlackjackTable`).
2. **Base Color** : branche un **Texture Sample** sur `BlackjackTable_BaseColor.png`.
3. **Normal** : **Texture Sample** (`BlackjackTable_Normal`) → nœud **Normal Map** (Tangent Space) → entrée **Normal** du **Default Lit**.

Sans ces deux textures, tu n’auras ni les bonnes couleurs ni le relief de la bake.

### 3. Assigner au Blueprint

1. Ouvre ton Blueprint de table.
2. Sélectionne le **Static Mesh** (composant).
3. **Materials** → slot **Element 0** → choisis `M_BlackjackTable` (ou une **MI_** dérivée).

---

## Rendu plus « propre »

- Le mesh reste **low poly** par design ; le détail vient surtout de la **normal map** + un bon **éclairage**.
- Après modification du script, **relance** la commande Blender puis **réimporte** le FBX dans Unreal.

---

## Résumé des réglages

| Problème | Piste |
|----------|--------|
| Échelle disproportionnée | **Échelle d’importation uniforme** = 1 à l’import, ou **Scale** sur le composant (ex. 0,01) |
| Pas de fenêtre Transform au réimport | Utiliser **nouvel import** du FBX ou **modifier les paramètres d’import** depuis l’asset |
| Pas de relief | Normal map branchée via **Normal Map** → **Normal** du matériau |
| Couleur bizarre / jaune | Aucun matériau assigné au slot du Static Mesh → assigner `M_BlackjackTable` |
| Trop « blocs » | Script mis à jour avec **shade smooth** ; sinon augmenter la résolution du mesh dans Blender plus tard |
