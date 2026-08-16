# AutoRun

Press **Z** while moving, let go of every key, and the character keeps going at the speed it was
already doing. From a standstill, **Shift + Z** sets off at a sprint and **Z** alone at a run. Press
**Z** again — or touch any movement key — and it stops.

Builds into `extra_autorun.pbo` and defines `VIGRID_AUTORUN`. It hooks vanilla and nothing else, so it
works on any DayZ server; the Battle Royale mod is not required.

It replaces DayZ-Expansion's AutoRun, one of the features that goes when the Expansion dependency is
dropped — the same job `Extra/SafeZone/`, `Extra/Map/` and `Extra/KillFeed/` did for their
Expansion counterparts.

**Discipline rule: nothing under `Extra/AutoRun/` may reference a `BattleRoyale*` symbol.** It carries
its own logger (`VigridAutoRunLog`), its own `stringtable.csv` (`STR_AUTORUN_*` keys), its own
`Data/Inputs.xml` and its own RPC namespace. Renaming `config.cpp` → `config.cpp.disabled` removes it
from the build with one rename.

## The key

`Z`, and **not** `X`. `kX` is vanilla `UAToggleWeapons` — change firearm fire mode, which also pokes
the zeroing HUD (`P:\bin\preset_keymouseprimary.xml:193`, consumed at
`P:\scripts\5_mission\mission\missiongameplay.c:551`) — so a bind there fires twice on every press.
`kZ`'s only vanilla binding is `UAMoveDown`, which sits under the preset's *DEBUG SPECIFIC / Freefly
Camera* block and is unreachable in normal gameplay. It is also what Expansion's own auto-run used, so
players coming off Expansion keep their muscle memory.

Free letters are nearly exhausted: `K`, `O` and `P` are the only ones vanilla leaves unbound, and Map
has taken `K` while Party has taken `P`. Rebindable in Options → Controls like every other bind.

⚠️ **While DayZ-Expansion is still in the load order, `Z` drives both auto-runs.** Expansion's
`UAExpansionAutoRunToggle` defaults to the same key — which is the point, players keep their muscle
memory — but until Expansion is actually dropped, one press engages both and their cancel rules fight.
Rebind Expansion's, or set `EnableAutoRun` to 0 in its general settings, for the transition period.

## What it does

The mechanism is `HumanInputController.OverrideMovementSpeed` / `OverrideMovementAngle`
(`P:\scripts\3_game\human.c:234-237`), held at `ENABLED` — documented as *"permanently active until
DISABLED is passed"* — and released with `DISABLED`. That is not a new API for this repo: the host
mod's own `PlayerBase.DisableInput` already uses exactly this pair with a speed of `0` to freeze
players.

**The speed is adopted, not fixed.** A press reads `hic.GetMovement()` and holds whatever the player
was already doing — sprint stays a sprint, a walk stays a walk. The moving cases need no modifier
check, because the sprint modifier is already folded into what `GetMovement` reports: Shift + W is
a `3`.

**From a standstill, the sprint modifier decides.** Shift + the bind sets off at a sprint; the bind
alone sets off at a run. Standing still is the only case with nothing to adopt, so it is the only
place `UATurbo` is read directly — and the arming latch below never watches `UATurbo`, so holding
Shift across the toggle cannot cancel what it just started.

**Sprint follows stamina.** `LimitsIsSprintDisabled()` is asked every frame; while it is true a
requested sprint is held at a run, and it goes back to a sprint by itself once stamina recovers,
because that flag un-latches on its own.

**Auto-run is applied on both sides.** The client holds the override locally *and* sends the resolved
speed to the server, which applies the same one. Vanilla's own `RPC_DAYZPLAYER_DEBUGSERVERWALK`
(`P:\scripts\4_world\entities\dayzplayerimplement.c:3723-3733`) is a client→server message whose whole
body is `OverrideMovementSpeed` on the server's copy of the player, which is the precedent for doing
it this way. The wire is edge-triggered: the server only hears about a change.

## What stops it

- a second press of the bind;
- **any of W / S / A / D being held** — but only once every movement key has been seen released at
  least once since the toggle. Without that latch the feature would be unusable, because the natural
  way to start auto-run is to press the bind *while running*, so W is still down on that very frame;
- a stance change (crouch / prone). A **jump is deliberately not a cancel** — vaulting a fence mid
  route is exactly what auto-run should survive;
- death, unconsciousness, entering a vehicle, a ladder, a climb, or swimming. Swimming would most
  likely work, being the same movement input, but it is one more untested axis; relaxing it is
  deleting one line in `VigridAutoRunClient.CanHold`;
- a host mod saying so — see below.

## Files

| File | Side | Role |
|---|---|---|
| `config.cpp` | — | One PBO, one `inputs=`, `defines[] = {"VIGRID_AUTORUN"}` |
| `Data/Inputs.xml` | client | `UAVigridAutoRunToggle`, default `kZ` |
| `stringtable.csv` | — | `STR_AUTORUN_KEYBIND_CATEGORY`, `STR_AUTORUN_KEYBIND_TOGGLE` |
| `Scripts/3_Game/VigridAutoRunConstants.c` | both | Input name, RPC names, speeds, `VIGRID_AUTORUN_MOVEMENT_ANGLE` |
| `Scripts/3_Game/VigridAutoRunLog.c` | both | `-autorun-*` CLI flags, `AutoRunLogLevel` in `serverDZ.cfg` |
| `Scripts/4_World/VigridAutoRunAPI.c` | both | The entire contract with a host game |
| `Scripts/4_World/Client/VigridAutoRunClient.c` | client | The feature: state, per-frame apply, cancel conditions |
| `Scripts/4_World/Server/VigridAutoRunState.c` | server | Who the server is holding an override for |
| `Scripts/5_Mission/Client/AutoRunMissionGameplay.c` | client | The 5th `modded class MissionGameplay` — glue only |
| `Scripts/5_Mission/Server/AutoRunMissionServer.c` | server | RPC registration, the handler, disconnect cleanup |

The feature lives in **4_World**, not beside its `MissionGameplay` glue in 5_Mission, for a structural
reason: `VigridAutoRunAPI` has to be 4_World so a host mod's server states can reach it, and a 4_World
class cannot name a 5_Mission type. Party solves the same problem the same way — state low, renderer
high.

## The API

A host game needs this for exactly one situation: it drives `OverrideMovementSpeed` itself. Auto-run
re-asserts its own override every frame, so without a way to say *not now* the two fight and the
freeze loses.

```c
#ifdef VIGRID_AUTORUN
    VigridAutoRunAPI.SetAllowed(false);   // client, before disabling input
    VigridAutoRunAPI.CancelFor(player);   // server, ON THE LINE ABOVE the freeze
#endif
```

Two call sites in the Battle Royale mod: `BattleRoyaleClient` where it applies the `SetInput` RPC, and
`4_BattleRoyalePrepare.DisablePlayerInput`.

⚠️ **`CancelFor` deliberately does not touch the controller**, and the server keeps a record of whose
override it owns. That record exists to stop one race: if a client's *"auto-run off"* message lands
*after* a host freeze, a handler that simply passed `DISABLED` would release the freeze and hand a
frozen player their controls back, with nothing in any log to say so. A release only happens for a uid
this addon still holds.

## Settings

None, deliberately — same as `Extra/SafeZone/`. The `config.cpp` rename is the kill switch and the key
is rebindable. Log verbosity is the only runtime knob: `-autorun-trace|-debug|-info|-warn|-none` on the
command line, or `AutoRunLogLevel` in `serverDZ.cfg`.

## Two things the sources get wrong, both settled by measurement

✅ **`VIGRID_AUTORUN_MOVEMENT_ANGLE = 0` is correct — measured 2026-08-16.** The character runs
straight ahead. It is worth writing down because the alternative reads better than it is: `0` is what
this repo's own `DisableInput` passes and is the obvious *"no deviation"*, but vanilla's own call sites
pass `1` instead, both the AI bot (`bot_hunt.c:145`) and the camera tools (`ctevent.c:50`), with no
documented unit anywhere. **Do not "fix" it to 1.**

✅ **Vanilla's walk pin does not reach auto-run — measured 2026-08-16, and there is deliberately no
`AddActiveInputRestriction` override.** Vanilla's entire body for the `INVENTORY` and `MAP` restrictors
is `UAWalkRunForced.ForceEnable(true)` — *"force walk on!"* (`missiongameplay.c:1001-1024`) — which is
what knocks a player down to a walk when they open their inventory. Read on its own that says
auto-run's speed must halve, and one build shipped an override countering it. It doesn't:
`OverrideMovementSpeed` sits downstream of the pin, so an auto-running sprint stays a sprint with the
inventory open. The counter-call was **deleted rather than kept "just in case"** — a no-op guarded on
`IsActive()` is indistinguishable from a working one, so it would have read as load-bearing forever.

## Testing

`LaunchOffline.bat` cannot answer the question that matters — offline the client *is* the server, so a
client-only override always works there. It is still worth one run to prove the PBO loads, the bind
resolves and the `script_*.log` is clean.

`LaunchLocalMP.bat 2` with `-br-autoconnect` is the real test. Watch the **second** client's screen for
the first player: a desync shows as the runner sliding or snapping back.
