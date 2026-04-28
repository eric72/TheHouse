# RTS & menus UMG

Documentation rapide des **widgets RTS** et des **menus contextuels** (objets posés, PNJ, ordres). La référence détaillée (tables propriétés Blueprint, hit-test, double-clic) est dans le guide développeur.

| Sujet | Où lire |
|--------|---------|
| Panneau principal vs menu objet | [GUIDE_FR § Interface RTS](../../Developer/GUIDE_FR.md#interface-rts--panneau-principal-vs-menu-contextuel) · [GUIDE_EN § RTS UI](../../Developer/GUIDE_EN.md#rts-ui-main-panel-vs-context-menu) |
| Menus PNJ + ordres + exemple C++ | Même section (sous-parties *Menus PNJ* / *NPC menus*) |
| Classes C++ | `Source/TheHouse/UI/` — `TheHouseRTSMainWidget`, `TheHouseRTSContextMenuUMGWidget`, `TheHouseNPCRTSContextMenuUMGWidget`, `TheHouseNPCOrderContextMenuUMGWidget`, `TheHousePlacedObjectSettingsWidget`, `TheHouseRTSUIClickRelay`, `TheHouseFPSHudWidget` |
| Propriétés sur le PC | Souvent sur **`BP_HouseController`** : `RTS Main Widget Class`, `RTS Context Menu Widget Class`, `NPCRTSContextMenuWidgetClass`, `NPCOrderContextMenuWidgetClass`, `PlacedObjectSettingsWidgetClass` |

Retour : [Features](../README.md) · [Documentation TheHouse](../../README.md)

---

## Personnel (palette staff) — vignettes

Le panneau **Personnel** (liste staff / recrutement) peut afficher une vignette :

- **Palette legacy** : `FTheHouseNPCStaffPaletteEntry::Thumbnail`
- **Recrutement (roster offers)** : `FTheHouseNPCStaffRosterOffer::Thumbnail` (par défaut : `StaffArchetype->StaffPortrait`)

Le widget C++ `UTheHouseRTSMainWidget` cherche d’abord la vignette sur l’offre de roster (si applicable), sinon sur la palette legacy.

---

## Personnel (recrutement “vivant”) — où configurer

Le système “Casino Inc” **recrutement** (liste de candidats avec étoiles / salaire / coût) est configuré sur le **PlayerController** (souvent `BP_HouseController`) :

- **`NPC Staff Recruitment Pool`** (tableau) : règles de génération (par métier).
- **`NPC Staff Roster Offers`** : liste **runtime** affichée (régénérée).

### Propriétés (regroupées) — PlayerController

Tout est volontairement regroupé sous **`TheHouse|UI|RTS|NPC|Recruitment`** sur `ATheHousePlayerController` :

- **Pool (règles) : `NPC Staff Recruitment Pool` (par slot)**  
  - `CharacterClass`, `StaffArchetype`, `OffersPerRefresh`  
  - `StarRatingMin/Max`, `SalaryMultiplierMin/Max`, `HireCostSalaryMonths`  
  - Noms candidats : `RandomDisplayNames` *ou* `RandomGivenNames` + `RandomFamilyNames`  
  - UI : `StaffCategoryId`, `StaffCategoryLabel`
- **Roster (liste affichée)** : `NPC Staff Roster Offers` (readonly)
- **Refresh auto (optionnel)**  
  - `Auto Refresh NPC Staff Recruitment Roster` (bool)  
  - `NPC Staff Recruitment Auto Refresh Interval Seconds` (float)

### Consommation des offres

Quand tu **embauches** (placement confirmé) depuis une ligne du roster :

- l’offre est **retirée** de la liste (elle ne doit plus être embauchable),
- si la liste devient vide, elle est **regénérée** automatiquement (si le pool n’est pas vide).

### Setup recommandé (exemple “tycoon”)

Sur `BP_HouseController` (Class Defaults) :

- **Pool** : ajoute 1 slot **GUARD** (et d’autres métiers plus tard) et mets :
  - `Offers Per Refresh` : **12** (ex. longue liste)
  - `StarRating Min/Max` : **1 → 5**
  - `Salary Multiplier Min/Max` : **0.85 → 1.15**
  - `Hire Cost Salary Months` : **3** (coût d’embauche ≈ 3 mois)
  - **Noms candidats** : laisse `Random Display Names` vide et remplis `Random Given Names` (20+) + `Random Family Names` (20+) pour des profils “uniques” à chaque refresh.
- **Refresh auto** :
  - `Auto Refresh NPC Staff Recruitment Roster` : **true**
  - `NPC Staff Recruitment Auto Refresh Interval Seconds` : **120** (2 min) — ajuste selon ton rythme de jeu

---

## Horloge “jour” + paie quotidienne (payroll)

Deux objectifs :

1. Afficher une horloge compressée (GTA-like).
2. Débiter automatiquement l’argent du joueur au **changement de jour** pour les employés en poste depuis ≥ 24h in‑game.

### Propriétés (regroupées) — PlayerController

Sous **`TheHouse|Time`** :

- `In Game Day Length Real Seconds` (ex. 900 = 15 min IRL → 24h in‑game)
- `In Game Hours Per Day` (par défaut 24)

Sous **`TheHouse|Economy|Payroll`** :

- `Enable Daily Staff Payroll` (bool)
- `Payroll Days Per Month` (int, défaut 30)

### Fonctions utiles (UI)

- `GetInGameClockText()` → `Jour X · HH:MM`
- `GetInGameDayProgress01()` → `0..1` (barre de progression)

### UI (WBP)

Dans ton WBP RTS (hérite de `UTheHouseRTSMainWidget`), tu peux binder :

- un `TextBlock` sur `GetTheHouseOwnerPC()->GetInGameClockText()`
- une `ProgressBar` sur `GetTheHouseOwnerPC()->GetInGameDayProgress01()`

### Setup recommandé (exemple “1 jour = 15 minutes”)

Sur `BP_HouseController` (Class Defaults) :

- `In Game Day Length Real Seconds` : **900** (15 min IRL = 24h in‑game)
- `Enable Daily Staff Payroll` : **true**
- `Payroll Days Per Month` : **30**

Où le modifier : **Blueprint PlayerController** (souvent `BP_HouseController`) → **Class Defaults** → catégorie **`TheHouse|Time`** → `In Game Day Length Real Seconds`.

Notes :

- Un staff embauché aujourd’hui **ne coûte pas** immédiatement : la première paie part au **jour suivant** (≥ 24h in‑game).
- Le salaire débité est **journalier** : \(MonthlySalary / PayrollDaysPerMonth\).

---

## Cycle jour/nuit (soleil) — lié à l’horloge in‑game

Un acteur C++ est fourni pour faire bouger le soleil en fonction de `In Game Day Length Real Seconds` :

- `ATheHouseDayNightCycleActor` (`Source/TheHouse/Environment/TheHouseDayNightCycleActor.*`)

### Mise en place (éditeur)

1. Dans ta carte (ou sous-niveau Lighting), **place** un `TheHouseDayNightCycleActor`.
2. Dans ses détails, assigne :
   - `SunLight` → ton `DirectionalLight` (le soleil)
   - optionnel : `SkyLight`, `SkyAtmosphere`, `ExponentialFog`
3. Ajuste :
   - `SunYaw` (direction est/ouest)
   - `SunPitchAtMidnight` / `SunPitchAtNoon`
   - `SunIntensityLuxAtNoon` (si pas de courbe)
4. (Optionnel) Assigne des courbes :
   - `SunIntensityLuxCurve` (0..1 → lux)
   - `SunLightColorCurve` (0..1 → couleur)
   - `SkyLightIntensityCurve`

### Performance

`SkyLight->RecaptureSky()` peut être coûteux. L’acteur a :

- `bRecaptureSkyLight` (toggle)
- `MinSecondsBetweenSkyRecapture` (throttle)
