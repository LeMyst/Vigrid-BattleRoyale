# Vigrid Safe Zone

A lobby truce: while active, players cannot shoot each other and cannot damage each other. It exists so
the pre-match lobby is not a free-for-all, and it replaces DayZ Expansion's Safe Zone for that purpose.

It hooks vanilla `PlayerBase` and `WeaponManager` directly, so it works on **any** DayZ server — Battle
Royale is not required.

|                 |                                                            |
|-----------------|------------------------------------------------------------|
| **PBO**         | `extra_safezone.pbo`                                        |
| **Side**        | both — deliberately, see below                              |
| **Stages**      | `3_Game`, `4_World` (no `5_Mission` — it needs no bootstrap) |
| **`defines[]`** | `VIGRID_SAFEZONE`                                           |
| **Requires**    | `DZ_Data`, `DZ_Scripts` (no CF — it uses no RPC)             |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                   |

It ships **no assets, no settings file, no stringtable and no RPC namespace.** It defaults to off, so
dropping the PBO on a server that never calls the API changes nothing.

## What it actually changes

Exactly two things, and the narrowness is the entire point:

1. **`WeaponManager.CanFire` returns false.** Pulling the trigger does nothing — no shot, no round
   consumed, no noise.
2. **`PlayerBase.EEOnDamageCalculated` returns false** for damage another player inflicted, so the hit
   is discarded before it is applied.

Everything else is deliberately left alone. Weapon raise, ADS, melee swings, reloading and user actions
all behave normally — players can still aim and still punch each other in the lobby.

Falls, drowning, infected, animals and the mod's own scripted zone damage all still land, because the
predicate resolves the damage source to a player (directly, via `GetHierarchyRootPlayer()` for a held
weapon, or by explosive type) and returns false for anything else — **including the victim as their own
source**, which is exactly how the mod's `DecreaseHealthCoef` zone damage surfaces.

## Why not Expansion's safezone

Expansion calls `hic.OverrideRaise(true, false)`, which kills ADS for every item including melee; hard
-returns false from `DayZPlayerMeleeFightLogic_LightHeavy.HandleFightLogic`, so melee swings do not even
play; and its `EEOnDamageCalculated` cancels *all* damage, not just PvP.

**If Expansion's safezone is left enabled it stacks on top of this addon and all those restrictions come
back.** Set `"Enabled": 0` in your mission's `Expansion/Settings/SafeZoneSettings.json`.

## Public API

```c
static void VigridSafeZoneAPI.SetActive(bool active)   // idempotent
static bool VigridSafeZoneAPI.IsActive()
```

Two call sites in the mod, both wrapped in `#ifdef VIGRID_SAFEZONE`:

| State | Call |
|---|---|
| `1_BattleRoyaleDebug.Activate()` | `SetActive(true)` — truce on for the lobby |
| `5_BattleRoyaleStartMatch.HandleUnlock()` | `SetActive(false)` — truce off |

The lift point is `HandleUnlock` rather than `Activate` because input stays locked through the warm-up
countdown — `HandleUnlock` is the first instant a player could actually shoot back.

## Design notes

**State is global, not geographic.** There is no zone module, no actor list and no per-tick
point-in-shape test. The server owns one static flag and mirrors it onto each player as the netsync
bool `m_VigridSafeZoneActive`.

**Netsync rather than an RPC broadcast**, specifically so a player joining an already-running lobby is
disarmed too; `OnConnect` / `OnReconnect` re-assert it unconditionally, because a rejoining player must
never end up armed because a sync was missed.

**`PlayerBase.c` and `WeaponManager.c` are deliberately unguarded** so both sides compile them:

- The client needs the flag locally so `CanFire` can refuse the trigger without a round trip.
- `RegisterNetSyncVariableBool` sits in the shared `Init()` override precisely so registration order is
  byte-identical on both sides — mandatory for netsync.
- Defence in depth: a client that patches out `CanFire` still lands zero damage, because
  `EEOnDamageCalculated` runs server-side.

## Caveats

- **A discarded hit produces no hit reaction.** A punch lands visually for the attacker but the target
  does not flinch. `TotalDamageResult` is getter-only, so scaling damage to zero is not available —
  cancelling is the only clean route.
- Expansion's safezone stacking, above. This is the one that bites in practice.
- `VIGRID_SAFEZONE_VERSION` is declared but never read — unlike KillFeed, this addon has no
  `MissionServer` boot line to log it.

## Logging

`-safezone-trace` / `-safezone-debug` / `-safezone-info` / `-safezone-warn` / `-safezone-none` on the
command line, or `SafeZoneLogLevel` in `serverDZ.cfg`. Diag builds default to trace. This is the only
external tuning knob the addon has.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the mod's two call sites are `#ifdef
VIGRID_SAFEZONE`, so it still builds without this addon — the lobby simply becomes a free-for-all.
