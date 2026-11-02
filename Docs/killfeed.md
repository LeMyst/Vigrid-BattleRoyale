# Kill feed (`Extra/KillFeed/`)

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

A standalone addon replacing the third-party `nulledkillfeed.pbo`. It builds into `extra_killfeed.pbo` and defines `KILLFEED`. It hooks vanilla `PlayerBase.EEKilled` itself, so it works on any DayZ server — Battle Royale is not required.

**Same discipline rule as `Party/`: nothing under `Extra/KillFeed/` may reference a `BattleRoyale*` symbol.** It carries its own logger (`KillFeedLog`), settings (`$profile:KillFeed\killfeed_settings.json`), `stringtable.csv` (`STR_KF_*`), layouts and RPC namespace (`RPC-KillFeed`). `KILLFEED_PREFIX` in `KillFeedConstants.c` is the single place the asset path appears.

**`ResolveActivatorName` asks for the recorded NAME first, and that ordering is the point.** Mapping the activator's uid back to a name by scanning `GetGame().GetPlayers()` can only answer for somebody still connected and still holding an identity — but an explosive routinely outlives its owner, which is the one case worth naming. A thrower who died or left yielded `""`, degrading their kill to an environmental death that `show_environment_deaths` could then suppress outright. `IsExplosive` tests the vanilla parents `ExplosivesBase` / `TrapBase`, not two concrete classes. Both reads stay defensive `EnScript.GetClassVar` calls — see [kill-attribution.md](kill-attribution.md) for why that works and why its return code must not be checked.

The Battle Royale mod talks to it **only** through `KillFeedAPI` (`Extra/KillFeed/Scripts/4_World/KillFeedAPI.c`), five call sites, each wrapped in `#ifdef KILLFEED`:

```c
#ifdef KILLFEED
    KillFeedAPI.NoteEnvironmentalDamage( player, KillFeedCause.ZONE );
#endif
```

- `SetActive(bool)` — the feed is off in the lobby (`1_BattleRoyaleDebug`), on from `5_BattleRoyaleStartMatch`, off again at `8_BattleRoyaleWin`.
- `NoteEnvironmentalDamage(player, cause)` — called at both `DecreaseHealthCoef` zone-damage sites (`6_BattleRoyaleRound`, `7_BattleRoyaleLastRound`). Scripted damage reaches `EEKilled` with the victim as their own killer, so without this hint a zone death is indistinguishable from starvation. Hints expire after `KILLFEED_HINT_TTL_MS` and are consumed by the death that uses them.

A row renders the killer's weapon **with its attachments** by spawning a client-local `ECE_LOCAL` copy, re-attaching the accessory classnames the server sent, and handing it to an `ItemPreviewWidget` — the same mechanism as the vanilla quickbar. At most `KILLFEED_MAX_ROWS` preview entities exist at once; each is `Delete()`d when its row expires.

`KillFeedSuppress` (`Scripts/4_World/Server/`) turns off other mods' kill feeds so a death is not announced twice, gated on the `suppress_other_killfeeds` setting. Each block is behind that mod's own define — currently only `#ifdef EXPANSIONMODKILLFEED`, which clears `GetExpansionSettings().GetNotification().EnableKillFeed`. That is Expansion's own documented switch, checked at both of its hooks in its modded `PlayerBase`, so clearing it is complete; the change is **in memory only**, so the admin's `NotificationSettings.json` is untouched and removing this addon restores the previous behaviour. Applied from `MissionServer.OnInit` plus one re-apply 10s later, in case another mod loads its settings afterwards.

NulledKillfeed is *not* covered — it is an obfuscated third-party PBO with no API to call. Remove it from `Workbench/ExtraPBOs/` or zero every `"active"` in its own `$profile:KillFeed\Settings.json`.

Note `$profile:KillFeed\` is also used by NulledKillfeed (`Settings.json`). The filenames differ, so they coexist, but deleting the folder to remove one wipes the other.
