# Vigrid Ear Plugs

One key cycles the local client through **Off → Light → Heavy**. The effects bus comes down *and* a
low-pass EQ goes on, so gunfire sounds muffled rather than merely distant. The level survives a
reconnect, and a badge on screen says the plugs are in.

It hooks nothing but vanilla `MissionGameplay`, so it works on **any** DayZ server — Battle Royale is
not required.

|                 |                                                                     |
|-----------------|---------------------------------------------------------------------|
| **PBO**         | `extra_earplugs.pbo`                                                |
| **Side**        | client — every `.c` file is `#ifndef SERVER`                        |
| **Stages**      | `3_Game`, `5_Mission` (no `4_World` — it owns no entity-stage code) |
| **`defines[]`** | `VIGRID_EARPLUGS` — declared for consistency, unconsumed            |
| **Requires**    | `DZ_Data`, `DZ_Scripts` (no CF — it uses no RPC)                     |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced                           |
| **Default key** | `J`                                                                  |

It ships **no API**: nothing in the host mod needs to talk to it, so there is no call site to guard
and no way for a server to reach in. Per-player state lives in
`$profile:Vigrid-EarPlugs\earplugs_client.json`.

## Credit and licence

The idea comes from **[DaemonForge/DayZ-EarPlugs](https://github.com/DaemonForge/DayZ-EarPlugs)**,
which is **GPLv3**. This repository is DSPL-SA, which adds non-commercial and DayZ-only restrictions
that GPLv3 §7 forbids adding to a combined work — so **no code and no asset from that mod is present
here.** Their four `volume_*.edds` icons in particular are why the indicator is a text badge built
from widgets rather than an image.

What is shared is the concept — one hotkey cycling muffle levels — which is not copyrightable
expression, and the vanilla API signatures, which are Bohemia's. Everything else is written against
declarations under `P:\` and this repo's own addons (`VigridMapPrefs` for the `$profile:` JSON,
`VigridMapMinimap` for the HUD, `KillFeedMissionGameplay` for the mission hook, `VigridSafeZoneLog`
for the logger). Same reasoning that blocked porting COT's `JMESPSkeleton` into this repo.

## What it actually changes

Two independent halves, both client-local. Either can be removed without touching the other.

**1. Bus volume, as a fraction of the player's own setting.**

| Bus | Scaled? | Why |
|---|---|---|
| `SetSoundVolume` | yes | World effects — the whole point |
| `SetRadioVolume` | yes | Diegetic world audio; plugged ears do not distinguish |
| `SetMusicVolume` | no | Non-diegetic, and it has its own Options slider |
| `SetVOIPVolume` | no | **This mod ships parties.** A player who plugs their ears must still hear their squad, or the feature is a competitive liability rather than a comfort setting |
| `SetSpeechExVolume` | no | Same reasoning |

Light is ×0.40 and Heavy ×0.12 of whatever the player has set. Never absolute values — see *Design
notes*.

**2. A real muffle**, via `Man.SetMasterAttenuation` (`P:\scripts\3_game\entities\man.c:41`) and two
new classes under `CfgSoundEffects >> AttenuationsEffects` in `config.cpp`, modelled on vanilla's
`BurlapSackAttenuation`. Light takes ~6–12 dB off the top two octaves; Heavy takes 15–21 dB off and
drops the mids 6 dB, with a touch of low-end lift — which is what plugged ears actually do, since
bone conduction keeps the bottom end while the canal is blocked.

Both presets zero the `Echo` section, unlike every vanilla preset. Vanilla is always modelling
something wrapped around your head — a sack, a car cabin, a concussion — and those reverberate.
Earplugs do not; they just take the top off.

## Design notes

### The baseline is measured, not read

The obvious source for "the player's own volume" is `g_Game.m_volume_sound`. It is wrong. Those five
fields are written in exactly **one** place — `DayZGame.DeferredInit`, `dayzgame.c:1130-1134` — and
never again, so they are stale the moment the player touches the Options audio sliders. Vanilla
inherits its own bug here: `MissionGameplay.OnPlayerRespawned` restores from them.

So **while the level is Off, whatever the engine reports is the baseline**, refreshed on the 4 Hz
tick and guarded on being alive and above zero. An Options change made with the plugs out is picked
up within 250 ms, with no hook and no event.

### The toggle writes, the reconciler only ever lowers

That asymmetry is the whole answer to a problem the reference implementation does not attempt.
Vanilla moves these buses out from under any mod that touches them:

| What | Where |
|---|---|
| zeroes all five on death | `dayzplayerimplement.c:861` |
| zeroes sound on uncon start | `playerbase.c:3534` |
| restores from `m_volume_*` on uncon stop | `playerbase.c:3581` |
| restores from `m_volume_*` on unpause / respawn | `missiongameplay.c:1626` |

and in this mod set the host's spectate entry restores all five as well. Left alone, the level and
the badge would go on claiming "Heavy" while the engine sat at full volume, and the player would have
to press the key three more times to resync.

Applying one rule — **the reconciler never raises** — makes all of it fall out:

- vanilla restored to baseline → `current > desired` → re-lower ✔
- vanilla zeroed for death/uncon → `current < desired` → leave it, it is theirs ✔
- level is Off → `current == desired` → no write at all ✔

and the one path allowed to raise is the player pressing the key, which is exactly what should
restore their audio when they take the plugs out.

Correction writes pass `time = 0` rather than a fade: a fade would keep `current > desired` true for
its whole duration and re-fire the write on every tick of it. The toggle's own fade is protected by a
short settle window for the same reason.

### The attenuation slot is shared, so we yield

`SetMasterAttenuation` is a single global slot with no stacking, shared with vanilla's unconscious,
burlap sack, flashbang and complete-deafness effects. Two rules keep everyone honest:

- only **write** when the slot is empty or already ours;
- only **clear** when it is ours — clearing blindly is how you un-muffle somebody who is unconscious
  or has a sack over their head.

Because the tick re-applies, a slot that a vanilla effect borrowed and has since given back is picked
up again within 250 ms without anyone having to notice. Flashbang re-asserts itself behind a defer
timer, so it wins; there is no flapping, because both sides only write when the slot is theirs or
empty.

### The badge is persistent

The reference implementation flashed an icon for about a second and faded it to nothing, which means
a player who plugs their ears and forgets is permanently half-deaf with no cue explaining why they
cannot hear footsteps. Here the badge stays up for as long as the plugs are in — full alpha for two
seconds after a change, then eased to an idle alpha it never drops below. A change **to** Off gets
the opposite treatment: nothing to show afterwards, so it flashes "OUT" and then hides.

## Caveats

- ⚠️ **`SetMasterAttenuation` is a *master* bus filter, so the muffle half may reach VOIP** whatever
  the bus table above says. If that turns out to be audible and unwanted, the two halves are
  independent by construction: drop the attenuation and keep the volume scaling.
- **Nothing stops a player muting gunfire.** That is inherent to any client-side audio mod and is not
  something a server can police.
- A player whose effects volume is already 0 gets a no-op. It is logged at startup so it is not
  mistaken for a broken addon.
- `J` is free across this mod's other four `Inputs.xml` files (Battle Royale `F1`–`F6` and the
  arrows, Party `P`/`T`/`Y`, Map `M`/`N`/`K`, AutoRun `Z`), but it is **not** strictly unbound in
  vanilla — only `K`, `O` and `P` are. `kJ` is `UABuldSlow`, under the **BULDOZER SPECIFIC** block
  of `P:\bin\preset_keymouseprimary.xml`: the Workbench model viewer, not the game, and its only
  reference in `P:\scripts` is the script console's suppression list (`scriptconsole.c:531`). That
  makes it unreachable in normal gameplay — the same standard AutoRun applied to `kZ`.
  **Community-Online-Tools is the one to check** if a bind ever does two things at once — its own
  `Inputs.xml` is what collided with Party's `Y`.

## Logging

`-earplugs-trace` / `-earplugs-debug` / `-earplugs-info` / `-earplugs-warn` / `-earplugs-none` on the
command line. Diag builds default to trace.

**No `serverDZ.cfg` key**, unlike SafeZone and Map, and its absence is deliberate: this addon runs
only on clients, and `ServerConfigGetInt` returns 0 on a client for every key, so such a branch would
be dead code that looks like a supported way to configure the thing.

At trace level every reconciler decision is logged **with its own reason** — `corrected`,
`already-at-target`, `below-target-leaving-alone`, `yielded-to:<name>`, `settling` — but only when
that reason **changes**. A guard that logs nothing cannot distinguish "this never ran" from "this ran
and threw everything away"; one that logs unconditionally at 4 Hz buries the change that mattered.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild. Nothing in the mod references this addon, so
there is no call site to guard and nothing else to do — **except deleting the stale
`extra_earplugs.pbo` and its `.bisign` by hand**, since an incremental `Deploy.bat` does not reliably
clean orphans.

To keep the addon but ship it without the muffle, return `""` from
`VigridEarPlugsLevels.Attenuation` — the volume half is untouched by that.
