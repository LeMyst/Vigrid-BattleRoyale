# Extra addons

`Extra/` holds independent, single-purpose sub-addons. Each is a small self-contained tweak that could
be lifted out of this repository and shipped on its own; none is required for the Battle Royale mod to
function.

**16 folders, 15 of which build** — `DisableFogChernarusPlus` is currently parked.

## How this folder works

- **One PBO per folder.** The build packs every folder that holds a `config.cpp` with no ancestor
  `config.cpp`. Keep exactly one `config.cpp` per addon and do not nest another below it.
- **Rename to disable.** `config.cpp` → `config.cpp.disabled` removes an addon from the build with a
  single rename. `DisableFogChernarusPlus/` is the worked example.
- **PBO names are lowercase**, derived from the folder path: `Extra/KillFeed` →
  `extra_killfeed.pbo`.
- **Discipline rule: no `Extra/` addon may reference a `BattleRoyale*` symbol.** That is what keeps
  each one extractable into a standalone mod as a build-plumbing job rather than a rewrite. Two addons
  currently break this rule and are marked below.
- **Side is set by the `#ifdef` on line 1 of each `.c` file**, not by the folder it lives in. An
  unguarded file compiles and runs on both client and server.

Adding a new one: create a folder with its own `config.cpp` and `Scripts/<stage>/`, and it becomes its
own PBO automatically. See the root `CLAUDE.md` for the script-module and stage conventions.

## The addons

| Addon | PBO | Side | What it does |
|---|---|---|---|
| [ChangeFeedbackURL](ChangeFeedbackURL/README.md) | `extra_changefeedbackurl.pbo` | client | Points the vanilla Feedback button at this mod's GitHub repo instead of Bohemia's tracker. **Not standalone** |
| [DefaultFullAuto](DefaultFullAuto/README.md) | `extra_defaultfullauto.pbo` | server | Weapons spawn already set to full auto where they have one |
| [DisableFogChernarusPlus](DisableFogChernarusPlus/README.md) | *disabled* | — | Removes fog on Chernarus+. Parked: it does nothing on Vigrid |
| [KillFeed](KillFeed/README.md) | `extra_killfeed.pbo` | both | On-screen death feed with weapon models, attachments and range |
| [LimitUnconsciousTime](LimitUnconsciousTime/README.md) | `extra_limitunconscioustime.pbo` | server | Force-wakes an unconscious player after 5 seconds. **Not standalone** |
| [Map](Map/README.md) | `extra_map.pbo` | both | Fullscreen map, HUD minimap, HUD compass, party-shared markers and zone circles |
| [MapSatellite](MapSatellite/README.md) | `extra_mapsatellite.pbo` | both | Config only — switches the map to satellite imagery, retunes its overlay layers and makes place names legible over it |
| [PreventPlayerModifiers](PreventPlayerModifiers/README.md) | `extra_preventplayermodifiers.pbo` | both | Disables hunger, thirst, broken legs and four diseases |
| [PreventWeaponRaise](PreventWeaponRaise/README.md) | `extra_preventweaponraise.pbo` | both | Stops the weapon being forced up against walls |
| [RandomMenuGear](RandomMenuGear/README.md) | `extra_randommenugear.pbo` | client | Re-dresses the main-menu character randomly on every menu show |
| [SafeZone](SafeZone/README.md) | `extra_safezone.pbo` | both | Lobby truce — no firing, no player-inflicted damage |
| [SpawnCarFull](SpawnCarFull/README.md) | `extra_spawncarfull.pbo` | server | Vehicles spawn with 30–100 % fuel, plus full coolant and oil |
| [SpawnWeaponChambered](SpawnWeaponChambered/README.md) | `extra_spawnweaponchambered.pbo` | server | Weapons spawn with a round chambered and a magazine where possible |
| [SpawnWithAmmoAndMagazine](SpawnWithAmmoAndMagazine/README.md) | `extra_spawnwithammoandmagazine.pbo` | server | Loot firearms spawn with magazines on the ground beside them |
| [SpawnWithBattery](SpawnWithBattery/README.md) | `extra_spawnwithbattery.pbo` | server | Battery-powered loot spawns with a battery fitted |
| [Vehicle3PPDefaultFix](Vehicle3PPDefaultFix/README.md) | `extra_vehicle3ppdefaultfix.pbo` | both | Forces common vehicles into the third-party Vehicle3PP whitelist |

## Runtime configuration

Most of these have no settings at all. The ones that do:

| Addon | Where |
|---|---|
| KillFeed | `$profile:KillFeed\killfeed_settings.json` — 4 options |
| Map | `$profile:Vigrid-Map\map_settings.json` (server) and `map_client.json` (per player) |
| MapSatellite | none, and it cannot have any — `MapWidget` exposes no satellite API, so the `config.cpp` rename is the whole control surface |
| SpawnWithAmmoAndMagazine | `serverDZ.cfg`: `BRDisableSpawnWithAmmo`, `BRMinSpawnAmmo`, `BRMaxSpawnAmmo` |
| LimitUnconsciousTime | `serverDZ.cfg`: `BRDisableUnconsciousness`, or `-br-disable-unconsciousness` on the command line |
| PreventWeaponRaise | `serverDZ.cfg`: `BRDisablePreventWeaponRaise` — server-read, mirrored to each client as a netsync bool |
| SafeZone | none — controlled entirely through its API |

## Compile-time defines

Only four addons declare a `defines[]`, which is what lets the mod guard its call sites with
`#ifdef`:

| Define | Addon | Used by the mod? |
|---|---|---|
| `KILLFEED` | KillFeed | yes — 5 call sites |
| `VIGRID_SAFEZONE` | SafeZone | yes — 2 call sites |
| `VIGRID_MAP` | Map | yes — client zone pushes and two server calls |
| `VIGRID_MAP_MINIMAP` | Map | no — read only by Map itself, as a build switch |
| `VIGRID_MAP_COMPASS` | Map | no — read only by Map itself, as a build switch |
| `RANDOM_MENU_GEAR` | RandomMenuGear | no — declared for consistency only |

`VIGRID_MAP_MINIMAP` and `VIGRID_MAP_COMPASS` are the odd ones out: they are not there for the host mod
to guard against, they are there so a build can ship with **no minimap** or **no compass** at all.
Comment either out of `Extra/Map/config.cpp` and that feature's class, its widgets and its keybind
handler leave the PBO, while the fullscreen map is untouched.

The other twelve declare none, so nothing can `#ifdef`-guard against them.
