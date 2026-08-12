# Kill Feed

An on-screen feed of recent deaths, showing who killed whom, with what weapon (rendered as a real 3D
model with its attachments) and at what range. It replaces the third-party `nulledkillfeed.pbo`.

It hooks vanilla `PlayerBase.EEKilled` itself, so it works on **any** DayZ server — Battle Royale is
not required.

|                 |                                                                     |
|-----------------|---------------------------------------------------------------------|
| **PBO**         | `extra_killfeed.pbo`                                                 |
| **Side**        | both — split by `#ifdef` per file, not by folder                     |
| **Stages**      | `3_Game`, `4_World`, `5_Mission`                                     |
| **`defines[]`** | `KILLFEED`                                                           |
| **Requires**    | `DZ_Data`, `DZ_Scripts`, **`JM_CF_Scripts`** (CF, for the RPC manager) |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                           |

## Settings

`$profile:KillFeed\killfeed_settings.json`, created on first boot.

| Key | Default | Meaning |
|---|---|---|
| `version` | `2` | migration marker, do not edit |
| `enabled` | `true` | master switch; `false` stops the server broadcasting entirely |
| `show_distance` | `true` | append the metre count on gunshot rows |
| `show_environment_deaths` | `true` | when `false`, every row with no killer is dropped — zone, falls, starvation, infected, animals |
| `suppress_other_killfeeds` | `true` | turn off other mods' feeds so a death is not announced twice |

The file is re-saved immediately after loading, so fields added in a later version appear in an
existing server's JSON on next boot. There is **no `$mission:` override** — profile only.

## Public API

The Battle Royale mod talks to this addon only through `KillFeedAPI` (`Scripts/4_World/KillFeedAPI.c`),
and every call site is wrapped in `#ifdef KILLFEED` so the mod still builds with the addon removed:

```c
static void KillFeedAPI.SetActive(bool active)
static bool KillFeedAPI.IsActive()
static void KillFeedAPI.NoteEnvironmentalDamage(PlayerBase victim, int cause)
```

Five call sites in the mod:

| State | Call |
|---|---|
| `1_BattleRoyaleDebug` | `SetActive(false)` — feed off in the lobby |
| `5_BattleRoyaleStartMatch` | `SetActive(true)` — feed on with the match |
| `6_BattleRoyaleRound` | `NoteEnvironmentalDamage(player, KillFeedCause.ZONE)` |
| `7_BattleRoyaleLastRound` | same, last-round damage tick |
| `8_BattleRoyaleWin` | `SetActive(false)` — match over |

### Why `NoteEnvironmentalDamage` exists

Scripted damage — the mod's `DecreaseHealthCoef` zone tick — reaches `EEKilled` with the victim listed
as their own killer. Without a hint, a zone death is indistinguishable from starvation. So the zone
damage sites push a hint, which the death consumes.

Hints expire after `KILLFEED_HINT_TTL_MS` (10 s) and are always cleared when read, so a stale hint can
never mislabel a later death. It is designed to be called on every damage tick.

## How a killer is attributed

`KillFeedDeath` follows vanilla's own `PluginAdminLog.PlayerKilled` cascade, in order:

1. No source, or the victim is their own source → consume a hint (defaults to `ENVIRONMENT`).
2. `Grenade_Base` / `LandMineTrap` → `EXPLOSIVE`, killer resolved from the activator.
3. Source is a weapon or melee weapon → killer is the item's hierarchy parent; `MELEE`, or `WEAPON`
   plus the rounded 3D distance.
4. Source is a player → `BAREHANDS`.
5. `ZombieBase` → `INFECTED`; `AnimalBase` → `ANIMAL`.

Then two filters: environmental rows are dropped if disabled, and a killer name equal to the victim
name is blanked — it reads as a bug.

Explosive attribution is deliberately **reflective** (`EnScript.GetClassVar(source, "m_ActivatorId", …)`)
so the addon never names the host mod's grenade classes. On a bare DayZ server that field is absent
and the row simply renders without a killer.

## Networking

One namespace, server → client only; the client never talks back.

- Namespace `RPC-KillFeed`, message `KF_Entry`, sent guaranteed with **no identity**, which is what
  makes CF fan it out to every client.
- Payload: killer, victim, weapon, attachments, distance, cause. Attachment classnames travel as one
  string joined by `;` — a character that cannot appear in a DayZ classname.
- CF dispatches by **method name**, so the handler on `KillFeedRPC` must stay named `KF_Entry`.

## Rendering

A row shows the killer's weapon with its attachments by spawning a client-local `ECE_LOCAL` copy,
re-attaching the classnames the server sent, and handing it to an `ItemPreviewWidget` — the same
mechanism as the vanilla quickbar. At most `KILLFEED_MAX_ROWS` preview entities exist at once, and each
is `Delete()`d when its row expires.

The weapon cell is sized per weapon from its config `itemSize`, clamped to 48–170 px, so a crossbow
does not leave 90 px of dead space before the victim's name.

## Suppressing other feeds

`KillFeedSuppress` turns off other mods' kill feeds when `suppress_other_killfeeds` is set. Each block
is behind that mod's own define — currently only `#ifdef EXPANSIONMODKILLFEED`, which clears
`GetExpansionSettings().GetNotification().EnableKillFeed`. That is Expansion's own documented switch,
and the change is **in memory only**, so the admin's `NotificationSettings.json` is untouched and
removing this addon restores the previous behaviour.

It is applied from `MissionServer.OnInit` plus one re-apply 10 s later, in case another mod loads its
settings afterwards.

**NulledKillfeed is not covered** — it is an obfuscated third-party PBO with no API to call. If you run
it, either remove it from `Workbench/ExtraPBOs/` or zero every `"active"` in its own
`$profile:KillFeed\Settings.json`.

## Logging

`-killfeed-trace` / `-killfeed-debug` / `-killfeed-info` / `-killfeed-warn` / `-killfeed-none` on the
command line, or `KillFeedLogLevel` in `serverDZ.cfg` (server only; negative disables). Diag builds
default to trace.

## Caveats

- `$profile:KillFeed\` is **shared with NulledKillfeed's `Settings.json`**. The filenames differ so
  they coexist, but deleting the folder to remove one wipes the other.
- Display tuning (`KILLFEED_MAX_ROWS`, `KILLFEED_ROW_SECONDS`, row geometry) is **compile-time
  constants, not settings** — the settings class is `#ifdef SERVER` and the client cannot read it.
- `KILLFEED_ICON_DEFAULT` is declared but never read; the skull icon actually comes from `image0` in
  `killfeed_row.layout`, so changing that constant does nothing.
- `KILLFEED_PREFIX` is the single place the asset path appears — keep it in sync if the folder moves.

## Disabling

Set `"enabled": false` in the settings JSON to keep the PBO but silence the feed. To remove it
entirely, rename `config.cpp` → `config.cpp.disabled` and rebuild — the mod's own call sites are all
`#ifdef KILLFEED`, so it still builds without this addon.
