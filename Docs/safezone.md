# Safe zone / lobby truce (`Extra/SafeZone/`)

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

A standalone addon replacing DayZ Expansion's Safe Zone for the lobby. It builds into `extra_safezone.pbo` and defines `VIGRID_SAFEZONE`. It hooks vanilla `PlayerBase` and `WeaponManager` directly, so it works on any DayZ server — Battle Royale is not required.

**Same discipline rule as `Party/` and `Extra/KillFeed/`: nothing under `Extra/SafeZone/` may reference a `BattleRoyale*` symbol.** It carries its own logger (`VigridSafeZoneLog`, CLI flags `-safezone-*`, `serverDZ.cfg` key `SafeZoneLogLevel`). It ships no assets, no settings file, no stringtable and no RPC namespace.

While active it changes **exactly two things**, and this narrowness is the whole point:

- `WeaponManager.CanFire` returns false, so pulling the trigger does nothing — no shot, no round consumed, no noise.
- `PlayerBase.EEOnDamageCalculated` returns false for damage that another player inflicted, so the hit is discarded before it is applied.

Everything else is deliberately left alone. Weapon raise, ADS, melee swings, reloading and user actions all behave normally — players can still aim and still punch each other in the lobby. Falls, drowning, infected, animals and the mod's own scripted zone damage all still land, because the predicate `VigridSafeZone_IsPlayerInflicted()` resolves the damage source to a player (directly, via `GetHierarchyRootPlayer()` for a held weapon, via `Transport.CrewDriver()` for a vehicle that ran them over, or by explosive type) and returns false for anything else — including the victim as their own source, which is how `DecreaseHealthCoef` surfaces.

This is why it is **not** a port of Expansion's safezone. Expansion calls `hic.OverrideRaise(true, false)`, which kills ADS for every item including melee, and hard-returns false from `DayZPlayerMeleeFightLogic_LightHeavy.HandleFightLogic`, which kills melee swings outright; its `EEOnDamageCalculated` cancels *all* damage, not just PvP. If Expansion's safezone is left enabled it stacks on top of this addon and those restrictions come back — set `"Enabled": 0` in the mission's `Expansion/Settings/SafeZoneSettings.json`.

Three things it does **not** cover, because none of them is a hit from another player: a **driverless** vehicle (nobody to blame, so it lands like any other physics hit); a flashbang's blinding and ringing, which is a client-side effect rather than damage (its explosion damage is discarded with every other explosive); and chemical gas, which works through agents from a contaminated area, exactly like the environmental gas zones.

State is global rather than geographic, so there is no zone module, no actor list and no per-tick point-in-shape test. The server owns one static flag and mirrors it onto each player as the netsync bool `m_VigridSafeZoneActive` — netsync rather than an RPC broadcast specifically so a player joining an already-running lobby is disarmed too; `OnConnect` / `OnReconnect` re-assert it.

The Battle Royale mod talks to it **only** through `VigridSafeZoneAPI` (`Extra/SafeZone/Scripts/4_World/VigridSafeZoneAPI.c`), two call sites, each wrapped in `#ifdef VIGRID_SAFEZONE`:

```c
#ifdef VIGRID_SAFEZONE
    VigridSafeZoneAPI.SetActive( true );
#endif
```

- On in `1_BattleRoyaleDebug.Activate()`, off in `5_BattleRoyaleStartMatch.HandleUnlock()` — the latter rather than `Activate()` because input stays locked through the warm-up countdown, so `HandleUnlock` is the first instant a player could actually shoot back. Defaults to off, so the PBO changes nothing on a server that never calls the API.
