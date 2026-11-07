# Spectating

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

Entered **in place at the moment of death** — the client never disconnects. Gated on
`spectate_enabled` in `general_settings.json`, which defaults **off**; with it off nobody is ever
offered spectating and the death screen keeps the old timing, quitting to the menu on its own after
`BR_DEAD_AUTO_QUIT_MS` (15 s, the same figure the pre-spectate `CallLater(LeaveServer)` used). The
death *screen* itself replaces the engine fade unconditionally — see below.

Two earlier attempts entered spectate on *reconnect* (the system removed in `e6a0e1b`, and the
unmerged `test-vpp-spectate` branch). That can never work here: `2_BattleRoyaleCountReached` locks
the server through the autolock webhook, and vanilla `MissionServer` calls `InvokeOnConnect` only
from `ClientNewEvent`/`ClientReadyEvent`, never from `ClientRespawnEvent`.

The mechanism is DayZ's own **`GetGame().SelectSpectator(identity, "BattleRoyaleSpectatorCamera", pos)`**
(`game.c:378`), called server-side. It resolves a **script class name, not a config class** —
vanilla's own `DayZSpectator` has no `CfgVehicles` entry anywhere in `P:\dz` — so
`BattleRoyaleSpectatorCamera` needs **no `config.cpp` entry**. (Contrast `CreateObject`, which *does*
resolve through config.) There is **no client → server spectate RPC at all**: targeting is automatic,
so there is no client request to authenticate.

**⚠️ KNOWN LIMITATION: a spectator can only see targets within roughly 1 km of their own corpse.**
`UpdateSpectatorPosition()` does **not** move the replication bubble for player entities. The camera
pushes it at vanilla's 0.5 s cadence and `BeginSpectate` calls `SelectPlayer(identity, NULL)` before
`SelectSpectator`, and neither re-centres anything: the bubble stays on the connection's own entity,
which is still the corpse (`GetGame().GetPlayer()` keeps returning it — see below).

**Established 2026-08-10 by deliberate, bidirectional measurement**, using the diag `TP Target` entry
to place the watched player at an exact radius. Exactly two `entity` transitions in the whole run,
each coincident with a teleport across the boundary:

| | |
|---|---|
| TP to **1200 m** | `entity 1 → 0`, sustained **100+ samples over 90 s** |
| target walks back to **1122 m** | still `entity=0` — no recovery |
| TP to **700 m** | `entity 0 → 1`, sustained |

Throughout all of it `UpdateSpectatorPosition` ran continuously (507 pushes) and the camera sat
within **4 m** of the pushed position — the exact condition that means "the push runs and has no
effect". `networkRangePlayers` is unset in `serverDZ.cfg`, so DayZ's default **1000 m** applies.

Client-side the symptoms are: **no character, no camera pan, but the nametag is still there** — the
name is a string from the `SetSpectateTarget` RPC, not from the entity, so it survives the dropout
and is the tell that this is replication rather than targeting.

**The bubble is centred on the corpse, and moving the corpse moves it.** Also measured, with the
diag `TP Corpse to Target` probe, and this one is a controlled experiment rather than a correlation:

```
20:26:38  TP Target -> 1200 m      corpse still at the death spot
20:26:41  entity 1 -> 0            target 1200 m from the death spot
20:27:06  TP Corpse -> target      corpse moves 1160 m, alive=0 (it is the body)
20:27:13  entity 0 -> 1            target STILL 1155 m from the death spot
```

The target never moved closer to where the player died — only the corpse moved — so distance-to-
corpse is the variable and nothing else is. Recovery took ~7 s, which is the juncture plus
re-establishing replication; a real implementation that moved the corpse *continuously* would never
cross the boundary and never pay it.

**A carried corpse leaves the world visually, and that is the cost to design around.** Tested on
ChernarusPlus: with the corpse moved 1200 m to the target, a second player sent back to the death
position found **no body there** — correctly, since it really did move — and had not seen it at the
carried position either. So the server's authoritative position moves (which is why the bubble
follows) while the entity's *replicated* position does not, and no client renders the body anywhere.

That is convenient in one way — nothing skates across the map for bystanders to see, so the 50 m
burial the removed carrier plan used is unnecessary — and fatal in another: **the victim's body and
its gear leave the match.** Any shipped version must therefore drop the inventory at the death
position *before* the body is ever moved. Vanilla `PlayerBase.DropAllItems()` (`playerbase.c:6681`)
is the tool: it walks the inventory and `ServerDropEntity`s each item, so it **creates nothing** —
the items already exist and are only moved out of the corpse. Called before the first carry, they
land at the death position for free, with no position arithmetic. It excludes anything inheriting
`SurvivorBase`, i.e. the body itself, and it does drop clothing too — harmless, since the corpse is
invisible.

The trade that leaves is presentational and is a **gameplay decision, not a technical one**: the
victim's loot becomes a pile on the ground rather than a lootable body. Nothing is destroyed and
nothing is created; it is only how the loot presents.

**This is implemented and validated live (2026-08-10).** `BattleRoyaleSpectators.CarryCorpse`, run
from the 1 Hz block in `Tick()`, tuned by six compile-time constants in `BattleRoyaleConstants.c`
(`BR_SPECTATE_CARRY_*`); set `BR_SPECTATE_CARRY_CORPSE` false to disable it and keep the old limit.
The first carry waits until the target is `TRIGGER_M` (250) away *and* nobody is within
`BYSTANDER_M` (50) of the body, with `FORCED_M` (750) as the backstop so camping a corpse cannot
hold a spectator's view hostage; the gear drops via vanilla `DropAllItems` before the body ever
moves, then it re-carries every `STEP_M` (250) of drift. Measured: carry fired at 252 m with
`0 left` on the body, and the target then tracked to **1237 m with `entity=1`** — past the old wall.

**A teleported target still derenders, and that is now the only way to see the old symptom.** The
same run shows a 19 s `entity=0` window immediately after a diag `TP Target` moved the watched
player 1136 m — and it happened while the target was *99 m* from the death spot, i.e. the opposite
of a range problem. Real players run rather than teleport, so the only production path that
reproduces it is the watched player taking an F2 unstuck. It self-corrects. Do not read such a
window as the carry failing.

There is **hysteresis**, which is what made this so hard to pin down: approaching from inside, a
target stays replicated out to ~1068–1200 m; once dropped, it does not return until well inside
(somewhere between 700 and 1122 m). That is why an early trace read "out at 1068, back at 720", and
why a session that never crossed the boundary — 39 clean samples to 929 m — looks like proof that
nothing is wrong. **A run that stops short of ~1100 m proves nothing.**

Two other things produce `entity=0` and must not be mistaken for this: a *retarget* to somebody far
away lands as one huge single-sample step, and *the match ending* freezes every field to identical
values while `pushes` keeps climbing (`EndSpectate` deliberately leaves the camera running).

**Why the push has no effect, decoded engine-side (serverkit [#22](https://github.com/LeMyst/dayz-serverkit/issues/22), 2026-11-07).** The server keeps one record per connection, and its bubble centre is what replication is computed around. `UpdateSpectatorPosition` **does** reach the server, and once `SelectSpectator` has flagged the connection as a spectator it **is** stored as that centre. Then, every tick and just before the bubble is recomputed, the server overwrites the centre with the position of the connection's **person entity**. `SelectPlayer(identity, NULL)` never detaches that entity: with NULL, `CGame::SelectPlayer` turns off the local camera and makes no network call at all, which is also why `GetPlayer()` keeps returning the corpse. A dead `PlayerBase` is still a `Person`, so the corpse always wins, and moving it moves the bubble. The bubble also re-centres only after 10 m of movement or 3 s, which is part of the ~7 s recovery after a carry. This was analysed on the 1.29 and 1.30 Linux servers, then on Windows 1.30 for serverkit PR #24: same mechanism, with MSVC laying the record out at different offsets.

**With the dzhooks `spectatorbubble` hook, the corpse is not carried at all.** The hook skips that overwrite while the connection is spectating, so the bubble follows the camera. It also injects `DZHOOK_SPECTATORBUBBLE`, and `BattleRoyaleNativeBubble.FollowsCamera()` (`Scripts/Server/4_World/NativeHooks/`) tests that define. When it is set, `CarryCorpse` returns at once, so the body stays where the player fell, gear included, and lootable. `CarryAnchorBody` still runs for admins: their live body is still their person entity, and carrying it under the camera keeps it at the bubble's centre, so the server never drops the admin's own controlled entity out of their bubble. Boot logs one `[NativeBubble]` line naming the mode.

**Tested 2026-11-07 on the diag rig, and it works.** The hook is serverkit PR #24 at `19caa78`, not merged yet. The rig was `DayZDiag_x64 -server` plus three diag clients, each player their own side:
- Client_B died, and the hook logged `spectating, bubble follows the camera` in the same second as `BeginSpectate`.
- The watched target stayed at `entity=1` in every 5 s sample: 12 samples at 1029–1098 m, 16 at **2590 m**, and 14 after diag teleports to 1200, 1350 and 1400 m. Not even the usual short teleport dropout appeared, and Myst saw the character, not just the nametag. Without the hook the same target vanished at about 1.2 km.
- No corpse carry was logged, and another player found B's body at the death spot.
- When the target died, B was retargeted to the last player and kept seeing them.
- B logged no script error in 6.5 minutes of spectating with its own body 2.6 km outside its bubble. That was the risk PR #24 named.

Still untested: looting the body, coming back from inside 1 km, and the F3 admin round-trip, which should log `bubble back on its person`. The test needed the diag rig because a retail server keeps BattlEye even with `BattlEye = 0;` and kicks local clients; [`native-hooks.md`](native-hooks.md) has the details.

**Do not "fix" this by creating a body to carry the bubble.** The diagnosis above is right; the
carrier is still wrong, and every part of *it* failed:

- `4005d62` retracted the original ~1000 m reading, on the grounds that the `entity=0` windows were
  *intermittent* and tracked camera movement of 450 m in 6 s — a target being **teleported**. **That
  retraction was itself an error**, and it is the reason this went round three more times: a
  teleporting target and a range cutoff produce the same `entity=0`, and the observation that one
  exists is not evidence the other does not. The tidy ~1000 m number was never a coincidence.
- **Neither way of creating that body survives a dedicated server.** `CreateObjectEx` gives
  *"Access violation. Illegal read … at 0x0"* in the engine's object setup, identical with and
  without `ECE_INITAI`. `CreatePlayer` then crashed on the first real spectate with
  *"Illegal write … at 0x9"* — fault bytes `F0 44 0F C1 71 08`, a `lock xadd [rcx+8]` refcount on a
  garbage pointer with `rcx = 1`. Neither is catchable in script. The classname is not the problem;
  `SurvivorM_Mirek` is `scope=2` in `dz/characters/data/config.cpp`.
- **`CreatePlayer`'s contract is the explanation**: its doc is *"Assign player entity to client (in
  multiplayer)"*, and vanilla's only `null`-identity call site is `missionbenchmark.c:366`, which has
  no clients. A NULL identity on a live MP server is out of contract. A *non*-NULL one is not an
  option either — the body would then be `ALIVE` and outside the current state, so `OnPlayerTick`'s
  force-logout kicks the very spectator it exists to serve.
- A boot-time probe reported `CreatePlayer` as safe and **that probe proved nothing**: it ran before
  any client existed, which is precisely the condition that makes the call legal. When a failure mode
  is a crash rather than an exception, the probe has to reproduce the real calling context.

`BattleRoyaleSpectators` (`Scripts/Server/5_Mission/BattleRoyale/Server/`) owns it. Three invariants:

- **It holds no object reference at all — only SteamID64 strings**, re-resolved at the moment of use,
  so a freed `PlayerBase` is never a failure mode. This is why the killer is stored as a uid: it must
  survive the killer's own death and disconnect. There are no exceptions to this.
- **A spectator is never revived and never re-added to `m_Players`.** `BattleRoyaleServer.OnPlayerTick`
  force-logs-out anyone outside the current state *and* `EPlayerStates.ALIVE`, and re-adding them
  would stall every `IsComplete()` and corrupt `br_position`.
- **The corpse is never deleted** — it *is* the `PlayerBase`, and it is the victim's loot. Vanilla
  `HandleBody` only deletes a body when the player was alive. The removed `e6a0e1b` implementation
  did `ObjectDelete(player)` with the comment *"this is for network bubble fix"* — destroying the
  victim's gear to solve a problem `UpdateSpectatorPosition` already solves for free.

Target selection is automatic and chained, resolved **at the moment spectating begins** (not at
death, so the chain reflects who is alive when you actually start watching) and re-resolved whenever
the target dies or disconnects:
**T1** a living teammate → **T2** the killer chain seeded at the last teammate to die (a solo player
seeds at themselves, so hop 0 is their own killer), walking `killer_uid` through dead killers →
**T3** the most recent kill whose killer is still alive → **T4** the living player nearest to where
this spectator fell → **T5** orbit the final circle. A `visited` set plus
`BR_SPECTATE_CHAIN_MAX_HOPS` guard the walk. T4 exists because a suicide or a pure zone death with
no kills anywhere otherwise resolved straight to an orbit of an empty circle.

Which tier fired is recorded on the entry as `resolved_tier` and appears as `(T1)`…`(T5)` on every
`Registered` / `Retarget` / `BeginSpectate` line. It is diagnostics only — nothing branches on it —
but without it the chain is five deep and only inferable from who you end up watching.
`m_LastResolveTier` is written at each of `ResolveTarget`'s five exits and read by the caller on the
very next line; it is a return value in all but name, so **never read it anywhere but immediately
after a `ResolveTarget` call**.

## The audience counter

A living player is shown how many people are currently spectating **them** — an eye-and-number row
on the HUD, hidden entirely at zero. It is the **last** row of `MatchInfoIconsPanel`, below the
zone arrow, and that is deliberate: the grid reflows when a child is hidden, so a row that comes and
goes mid-match shifts everything under it — put at the bottom it can move nothing, which is what
keeps the clock and the distance arrow still. Gated on `show_spectator_count`
(`general_settings.json` v6, defaults **on**); `spectate_enabled` already decides whether anyone can
spectate at all, so on a server that never offers it this is never reached.

⚠️ **The filter is `entry.is_admin` — the SESSION TYPE — and never membership of
`admins_steamid64`.** An admin in *admin* spectate must not be counted: an operator watching is not
part of the game, and surfacing it tells a player the exact moment they are being observed, the same
leak the invisible anchor body and `SetMemberHidden` exist to prevent. But an admin who competed,
died and took **ordinary** spectate is a real audience member and *does* count — key it on the
roster instead and the number shown depends on who is on an admin list rather than on what sessions
exist. `is_admin` is written in exactly one place (`BeginAdminSpectate`) and means precisely "this is
an operator session". Two further exclusions: `pending_enter` (still on the death screen — the camera
is not attached, they may press Quit, and `OnDeath`'s `target_uid` is the *provisional* one
`BeginSpectate` re-resolves) and `target_uid == ""` (the T5 orbit, which has nobody to charge it to).

⚠️ **`PushAudienceCounts()` is called from the TOP of `Tick()`, above BOTH of its early returns, and
that position is load-bearing.** `m_Spectators.Count() == 0` is the *last spectator leaving* — the
one moment the watched player has to be told zero — and `m_Ended` is `EndAll()` having cleared the
table, so below either guard the count could be raised and never lowered, and **the winner would
hold "12 watching" over the win screen for the rest of the session.** Nothing else needs a special
case: `EndAll()` empties `m_Spectators`, so the tally simply comes back empty and the drain half
pushes the zeros.

It is a **recompute plus a diff**, not an incrementally maintained number, because the count is a
many-to-one aggregate: a `Retarget` moves one spectator and changes **two** players' counts with no
entry added or dropped, `BeginSpectate` clears `pending_enter`, and an admin session supersedes an
ordinary one. `m_AudienceSent` (watched uid → count last pushed) exists solely because a recompute
can see who *has* an audience but not who has just **stopped** having one. The admin switch gates the
tally rather than the pass, so turning it off mid-match pushes everyone a zero instead of freezing
the row on its last value.

**Nothing identifying goes on the wire** — `SetAudienceCount` is a `Param1<int>` sent per identity,
so a player learns they are being watched without learning who is dead. Diag: *HUD & Menus* → **Force
HUD** + **Fake Audience** is the only way to reach the element offline, where `SERVER` is undefined
and no spectator can exist; its range reaches 0 because 0 is the hidden state. Server side, **Log
Spectators** dumps `m_AudienceSent`.

## Admin spectate

A second, separate session type on the same machinery: a free camera, target cycling, and a
name/health overlay, for `admins_steamid64` members only. Gated on `admin_spectate_enabled`
(`general_settings.json` v5, defaults **on** — membership of the admin list is the real gate, so
defaulting it off would only mean every operator edits a file before an admin tool works).

**One rule governs the whole feature: admin spectate requires a NON-PARTICIPANT — alive, holding a
body, and absent from `m_Players`.** A competing admin is refused, because a competitor who can
freecam the map is indistinguishable from a cheat. There are exactly two ways to be a
non-participant, and **the admin respawn is the bridge** between them:

```
Admin competing in the match        F3 -> refused ("still in the match")
        | dies
Dead admin: death screen/spectate   F3 or the ADMIN MODE button -> respawn, then camera
        v
NON-PARTICIPANT ADMIN               F3 toggles camera <-> body, <-/-> cycle, F5 mode
   ^
Admin who connected mid-match       already lands here (OnPlayerConnected)
```

`BattleRoyaleSpectators.AdminEligibility()` **is** that rule, returning one of four
`BR_ADMIN_*` verdicts, and every admin RPC consults it rather than re-deriving it. Keys are F3
(toggle), F5 (mode), ←/→ (cycle), all in `Data/Inputs.xml`.

Five things worth knowing:

- **`AdminRespawn` creates an entity, which invariant 2 otherwise forbids — and does not break it.**
  The new body is never added to `m_Players`, so the roster count, every `IsComplete()` and
  `br_position` are untouched; the admin's placement, leaderboard entry, death record and corpse all
  stand. It mirrors vanilla `MissionServer.CreateCharacter` (`missionserver.c:486-495`) inline rather
  than calling it, because that writes the mission's `m_player` and because this mod overrides
  `EquipCharacter` to apply the *lobby* loadout. **The two recorded crashes do not apply**:
  `CreateObjectEx` faulted at 0x0, and `CreatePlayer` faulted at 0x9 with a **NULL** identity, which
  is out of contract. This passes a real one. The other recorded objection — "the body would be
  ALIVE and outside the state, so `OnPlayerTick` force-logs-it-out" — was about the carrier body;
  here that is the goal, and `IsLateJoinExempt` now covers it.
- **`IsLateJoinExempt` consults `admins_steamid64` directly**, not just `a_LateJoinExempt`. That
  array is only populated by `OnPlayerConnected`'s mid-match branch, so an admin who connected during
  the *lobby* and later held no state was never on it and got kicked. Latent bug, fixed here because
  the respawn depends on it.
- **`ResolveAdminTarget` is a separate resolver, not a sixth tier.** `ResolveTarget` opens with
  `m_Deaths.Get(spectator_uid)` to find the spectator's own party and killer chain; a non-participant
  admin has no death record, so all five tiers degrade to the T5 orbit **without saying so**. The
  admin order is: the killer of whoever just died (if alive) → next living player in cycle order →
  orbit. Cycle order is `GetPlayers()` **sorted by uid**, so it cannot renumber itself under the
  admin as the roster compacts.
- **The camera carries the admin's own body under itself** (`CarryAnchorBody`, fed by a 2 Hz
  `AdminSpectateCamPos` RPC), which is what moves the replication bubble and defeats the ~1 km limit
  described above. Unlike `CarryCorpse` it skips `DropAllItems` — it is the admin's own gear, handed
  back on exit. **The push must run in every mode, not just FREE.** It was FREE-only at first, so a
  following admin never updated `cam_pos`, the carry measured zero drift from the respawn point and
  never fired, and the bubble stayed where they respawned — the symptom being that the admin opens
  the camera and sees *no players at all*.
- ⚠️ **A carried LIVE body is visible and its teleports replicate — AND THIS IS STILL UNSOLVED.**
  The "a carried body's replicated position does not move" measurement in the section above was
  taken on a **corpse** and does **not** carry over: a live body is still simulated, so other
  players watch an admin-shaped character slide across the map with the camera, and the admin sees
  their own body standing next to whoever they are following (observed 2026-08-11).

  **`SetInvisibleRecursive(true)` called server-side does NOT fix it** — tried, shipped, and
  measured still visible on 2026-08-11. The likely reason is that `SetInvisible` is a local render
  flag rather than replicated state: COT only ever calls it **client-side**, on the local player's
  own model (`JM/COT/.../Player/DayZPlayerImplement.c:208`). It is left in the code because it is
  harmless, not because it works.

  **The known-good route is COT's `PlayerBase.COTSetInvisibility()`**, whose `JMInvisibilityType`
  includes `DisableSimulation` as well as `Interactive` (`JMPlayerModule.c:1572-1628`) — i.e. real
  invisibility needs the simulation disabled, not just a render flag. COT is a hard dependency and
  this is a plain API call behind `#ifdef JM_COT`, so it carries no licence obligation (COT is
  CC BY-SA 4.0; *calling* it is not adaptation). Untried as of this writing.

- **A spectating admin is hidden from their own party's state feed**, via
  `VigridPartyAPI.SetMemberHidden(uid, bool)` — the fix for a leak where a respawned admin showed to
  their teammates as a live member with a compass caret pointing at wherever the camera had carried
  their body (observed 2026-08-11, fixed 2026-08-12). Note `SetHudSuppressed` had *looked* like the
  fix and only ever addressed the **admin's own** screen; the leak was on everyone else's, which
  nothing client-side can reach because it is the server's push putting the position on the wire.
  Three things about it:
  - **A hidden member is presented exactly as an OFFLINE one** — zeroed position, zeroed flags — and
    that is the whole implementation rather than a new flag. Every consumer already routes through
    `IsMemberVisible` / `IsMemberOnline`, both keyed off the `ONLINE` bit, so this suppresses the
    world nametag, the compass caret and the map's team layer at once with no client change. They
    stay a full member otherwise: on the roster, listed by name, still counted by `GetGroupCount`.
    The visible cost is that teammates see them as "Offline".
  - **Party is told nothing about spectating.** The API is documented as "a host mod can put a player
    somewhere that is not where they are playing from", which keeps the discipline rule intact.
  - ⚠️ **The clear is the dangerous half** — a member left hidden reads as permanently offline for
    the rest of the match. Every removal now funnels through `BattleRoyaleSpectators.DropEntry`,
    which releases it; there were four exit paths and a fifth in `EndAll`, which skips
    `EndAdminSpectate` for an admin whose identity has already gone and then `Clear()`s the map. Add
    no sixth: drop entries through `DropEntry` and nowhere else.

  Verified live 2026-08-12, and the useful part is *which* paths were exercised. The hide fires
  before `BeginAdminSpectate` returns, so the first `VP_TeamState` after a respawn already carries
  the admin as offline and there is no window for a caret. It also fired on an entry with **no
  preceding `AdminRespawn`** — the mid-match-joiner branch — which confirms it hangs off the session
  rather than off the respawn. The release was seen on **two non-clean paths**: a disconnect, and a
  match end, both landing `SetMemberHidden … visible` immediately before the entry was dropped.
  Those are the ones that would have stranded a member as permanently offline.
- ⚠️ **Do not hand the body back in the same frame as the exit teleport.** It crashed the client in
  vanilla `DayZPlayerCamera1stPerson.UpdateUDAngleUnlocked` (`dayzplayercamera_base.c:132`): the body
  has never been simulated — created by `CreatePlayer` and dropped from the selection immediately —
  so vanilla's camera initialises against a player that is still mid-juncture. `SelectPlayer` is
  deferred `BR_ADMIN_EXIT_SELECT_DELAY_MS` behind the teleport, in `FinishAdminExit`.
- **Party's HUD is suppressed for an admin session only**, through
  `VigridPartyClientAPI.SetHudSuppressed()` — a switch the host flips, so `Party/` still names no
  `BattleRoyale*` symbol. Without it the party nameplates stack with the admin ones over the same
  character and the roster panel describes a party the admin is not currently playing in. An
  *ordinary* spectator keeps their party HUD: following a teammate with it live is the wanted
  behaviour.
- ⚠️ **The focus counter is the trap.** `EnterSpectate`'s three `ChangeGameFocus(-1)` calls exist to
  undo the `LockControls(true)` that `SimulateDeath` performed. An admin entering spectate **alive**
  never ran `SimulateDeath`, so releasing anyway drives the additive counter negative — which breaks
  input with no error, no log line and nothing on screen. They are gated on `b_DeathLocked`, set by
  `ShowDeadScreen` via `BattleRoyaleRPC.death_locked` (the same 4_World→5_Mission stage hop
  `dead_placement` uses) and consumed exactly once. `LeaveSpectate` deliberately adds **no**
  symmetric `+1`.

### The admin overlay

The overlay (`BattleRoyaleSpectatorTags.c` + `spectator_tag.layout`) shows name, health bar,
distance, kills, party colour and a highlight on the current target. It is fed by the
`SetAdminPlayerList` RPC — **per-identity to registered admin spectators, never broadcast**, because
it carries SteamID64s, which `SetLeaderboard` deliberately keeps off the wire for exactly that
reason. That push is also why **tags work at any range while models do not**: positions come from the
server, bones do not.

**Names are coloured per PARTY, and that field is NOT `VigridPartyPalette` (#276).** The wire carries a
match-local party index (`admin_parties`, was `admin_slots`) which
`BattleRoyaleTeamColour.ForParty` turns into a generated pastel. Five things are load-bearing:

- ⚠️ **The bug was a SEMANTIC one on a field that already existed and already drew.** The server filled
  it with `VigridPartyAPI.GetMemberIndex` — the player's slot *inside* their own party — so every
  party's first member came out amber and the overlay answered "which of my teammates is that" to
  somebody who is in none of the parties. Nothing was missing; the wrong index was being sent.
- **The index is keyed on `GetPartyId()`**, a stable string, into `m_PartyColourIndex`, assigned
  first-seen. Deliberately **not** a position in `GetGroups(roster)`, which re-partitions the living
  population every push — a team's colour would change whenever somebody in an earlier group died. A
  party reduced to one living member keeps its colour: they are the last of a team, not a solo.
- **The palette is not reused because it cannot answer this question.** It is eight hues wrapping on
  `slot % 8`, right for a party of at most 16 that you are in, wrong for a match holding thirty
  parties where two sharing amber *is* the question. Both still exist and neither should be pointed
  at the other's job.
- **The walk is the GOLDEN ANGLE with six sat/val tiers, and the tier count is not a round number.**
  A golden-angle walk puts its closest hue pairs at Fibonacci offsets (8, 13, 21), and a tier count
  dividing one of them leaves that pair separated by hue alone. At the first-written 3 tiers, parties
  21 apart measured **dE 2.9** in CIE Lab — the just-noticeable threshold, i.e. indistinguishable. Six
  divides none of them. The twelve tier values were *searched*, not picked; measured over 30 parties:
  worst pair anywhere **dE 13.0**, worst adjacent pair **dE 46.4**, nearest to the solo white
  **dE 20.6**. They are one package — changing one saturation moves the worst-case pair elsewhere.
- ⚠️ **Thirty distinguishable colours do not exist and the code says so.** Categorical colour tops out
  around ten to twelve. What is guaranteed is that no two parties share a colour and that adjacent
  indices are far apart, which is what "are those two on the same side" needs. Do not "fix" this by
  adding hues. `BR_SPECTATE_TAG_TARGET_COLOUR` is now a **fully saturated** amber precisely because
  every generated colour is a pastel (sat ≤ 0.50), so the follow highlight can never collide.

**A nearby body gets a name too (#278)**, from a second RPC, `SetAdminDeadList` — names and positions
only, no uids and no party index. Four things about it:

- ⚠️ **The positions are where each player FELL, not where the body is now**, and that is correct
  rather than lazy. `CarryCorpse` moves a spectator's corpse hundreds of metres, but a carried body's
  *replicated* position does not follow (measured, above), so no client renders it anywhere — tagging
  the live server position would put a name in empty air and leave the fight unlabelled.
- That is also what makes the set **append-only**, which is why it rides a 2 s clock with a count
  comparison rather than the 500 ms player push. A keepalive covers an admin entering spectate in a
  lull. 59 names and positions at 2 Hz would be the largest recurring payload in the mod.
- **Proximity is a rule, not a clutter cap**: `BR_SPECTATE_CORPSE_TAG_RANGE_M` is 40 m against the
  skeleton overlay's 500. The skeleton says a fight happened over there; the tag says who it was.
- Corpse tags are dim, carry no health bar, sort *below* every living tag, fade over the last 8 m and
  are **not edge-clamped** — an off-screen body is clutter where an off-screen living player is worth
  knowing about. They are deliberately not team-coloured.

**Both screen-space overlays are suppressed while the fullscreen map is open (#279)** — the tags and
the COT skeleton canvas, via `BattleRoyaleClient.IsFullscreenMapOpen()`. ⚠️ **The skeleton guard sits
BELOW the canvas `Clear()`, not above it**: a `CanvasWidget` keeps its draw list, so an earlier return
burns the last frame of skeletons into the map for as long as it stays open. The map plots the same
players itself — see *the admin player layer* in [map.md](map.md).

### Skeletons (F6)

**Skeletons are COT's renderer, called rather than copied — but from OUR loop, not COT's.** F6
(`UADayZBRSpectateSkeleton`) is the one admin key with **no server half**, because it changes only
what this client draws. `BattleRoyaleClient.UpdateSkeletonOverlay` runs each frame and calls
`JMESPSkeleton.Draw(human, canvas, thickness)` — a public static that reads bone positions in world
space and writes into COT's `JMESPCanvas`.

⚠️ **Two builds went into driving COT's own loop instead, and neither drew anything.** The first set
`JMESPModule.SetDrawPlayerSkeletonsEnabled(true)`; the second added `esp.EnableUpdate()`. Each time
the log proved the flag was set, the `ESP.View` permission granted and the canvas present — so
`JMESPModule.OnUpdate`, which is what actually draws, was simply never ticked for us. Do not go back
to the flag; it is not the mechanism, and the tidy reading of COT's source that says it should be is
the same reading that cost those two builds.

Four things to keep:

- **Range is ours.** COT culls each subject at `ESPRadius` — 200 m by default, compared against the
  projection's *depth*, and an admin's own COT setting — which is well inside where a spectating
  admin watches from and would likely have suppressed everything even had the loop been ticking.
  `BR_SPECTATE_SKELETON_RANGE_M` is 500. COT's own `ESPRadius` is deliberately **not** written to; it
  outlives this session and is not ours to change.
- **Clearing is ours.** A `CanvasWidget` keeps what was drawn until something clears it, so the
  canvas is cleared every frame *before* the redraw, and again on toggle-off and in `LeaveSpectate`.
  Skeletons left running would follow the admin back into their own body and give a non-participant
  an x-ray view of the match — the exact thing the non-participant rule exists to prevent.
- **The living and the dead are filtered differently, because only one of them can be roster-checked.**
  A LIVING subject must be on the `admin_uids` roster the server already pushes for the overlay tags,
  and that is what keeps the admin's **own anchor body** out of it: the body is created server-side
  and reaches its owner's client as a *remote* entity — the connection's selected object is the
  spectator camera, not the body — so `ClientData.m_PlayerBaseList` holds it like anybody else, and
  testing it against `GetGame().GetPlayer()` cannot work, because that inequality is precisely why it
  was inserted. A non-participant is absent from `admin_uids` by construction. A CORPSE is drawn
  unconditionally: `admin_uids` carries only the living, so a body fails that test by definition, and
  there is nothing to gate it on anyway — a corpse marks where a fight happened, which is the point.
  Corpses do stay in `ClientData.m_PlayerBaseList` (the mod never deletes a body), verified rather
  than assumed. Measured with one corpse and one live player in view:
  `population=3 notplaying=1 corpses=1 drawn=2`.
- ⚠️ **Corpses are drawn by US, and the colour is the whole reason.** `JMESPSkeleton.Draw` takes no
  colour — it derives one from `GetHealthLevel()`, and a dead body is `STATE_RUINED`, which COT
  paints `0xFF232323`. Near-black is the worst possible colour for the one job a corpse marker has.
  `JMESPCanvas.DrawLine` *does* take a colour, so the corpse pass goes straight to the canvas at
  `BR_SPECTATE_SKELETON_CORPSE_COLOUR` and 2 px (thicker because a body lies flat and foreshortens).
  Living players still go through COT's renderer, whose health colouring is worth having.
  **The bone chain is derived from vanilla's rig, not transcribed from COT's `s_Limbs`** — calling
  `JMESPSkeleton.Draw` is interoperation, copying its table would be adaptation. The pairs come from
  the bone names vanilla registers per damage zone in `BleedingSourcesManagerBase.Init`
  (`P:\scripts\4_world\classes\bleedingsources\bleedingsourcesmanagerbase.c:23-61`); it shows, since
  ours carries the shoulders and the full spine and stops at the toes rather than the finger bones.
  `BR_SPECTATE_SKELETON_CORPSES` compiles the pass out.
- **COT's `ESP.View` permission is no longer consulted**, since nothing calls COT's gated setter any
  more. The gate is this mod's own: `admins_steamid64`, non-participant, and in a spectate session.

This is also the licence answer: porting `JMESPSkeleton` is blocked — COT is CC BY-SA 4.0, this repo
is DSPL-SA, and BY-SA §3(b) forbids relicensing adapted material under added restrictions — but
*calling* a published API is interoperation, not adaptation, and carries no obligation at all.

## Testing spectate

**Testing it needs the diag menu's Spectate submenu** (`DIAG_DEVELOPER`, so both `DayZDiag_x64`
sides). **Kill Me** is the one that matters: without it, reaching the death screen at all needs a
second client to land a kill, which made every spectate test a three-client test. It sends
`KILL_SELF`, and the server does `SetHealth("", "Health", 0)` on the sender rather than calling
`RecordDeath` directly — `EEKilled` is the hook under test, so a shortcut would exercise only the
parts that were never in doubt. That also means the killer resolves to the victim themselves, so
**Kill Me reproduces T4 specifically**; a real kill by another player is still the only way to reach
T1 or T2. **Log Spectators** dumps the table with each entry's tier and whether its identity still
resolves ("registered but gone" is the shape of a leak). **Spectate Enabled** flips
`spectate_enabled` in memory only — `Load()` re-saves on next boot, so a diag toggle must never
become a persisted setting.

## The death screen

**The death screen is a `UIScriptedMenu`** (`MENU_BR_DEAD`, `Scripts/Client/5_Mission/GUI/DeathScreenMenu.c`
+ `GUI/layouts/death_screen.layout`), not `UIManager.ScreenFadeIn` — the engine fade is a proto
native taking a string and two colours and has no widget tree, so it cannot carry buttons. It offers
**Spectate** (only once the server has sent `SetSpectateOffer`) and **Quit to menu** (always, so a
server with `spectate_enabled` off still has an exit). Pressing Spectate sends the one client→server
RPC in the feature, `RequestSpectate`, which carries **no payload**: the server resolves the actor
from `sender` and will only act on a uid that is already a registered, still-pending spectator.
Pressing nothing lets `BR_SPECTATE_ENTRY_DELAY_MS` start it anyway.

The screen runs **one of two countdowns**, and which one is the whole of its state machine. While an
offer stands it mirrors the server's `BR_SPECTATE_ENTRY_DELAY_MS`; with **no** offer standing it runs
`BR_DEAD_AUTO_QUIT_MS` and quits to the menu itself. The second case is not just `spectate_enabled`
being off — `EndSpectate` **withdraws** a standing offer, because `EndAll()` also reaches players who
are still sitting on the death screen. Without that withdrawal they were left pressing a Spectate
button the server had already stopped honouring (`RequestSpectate` returns at its `m_Ended` guard
silently), watching a countdown to an entry that would never happen.

**The death screen is driven from `BattleRoyaleClient.Update()`, not from `UIScriptedMenu.Update()`.**
The engine calls `UIScriptedMenu.Update()` exactly **once** for this menu — established by logging
the first five frames and never seeing more than one. Anything per-frame (the countdown, holding the
cursor) therefore has to hang off `BattleRoyaleClient.Update()`, which runs unconditionally from
`MissionGameplay.OnUpdate` outside vanilla's `m_LifeState == ALIVE` gate. Its `Tick()` is the entry
point. Three traps in the layout, all of them about the backdrop:

- **A `PanelWidgetClass` with no `style` paints nothing.** `color` alone is not a fill — the widget
  still lays out and still hosts its children, so the buttons and text draw perfectly over a
  completely transparent background, which is exactly how it presents live. `style DayZDefaultPanel`
  is what gives a panel its fill; `leaderboard.layout` is the working reference in this mod, same
  construct. Removing that style to chase the edge gap below cost the whole backdrop.
- **Widgets inset themselves to the safe zone**, so a `size 1 1` backdrop leaves a gap at the screen
  edge unless it sets `keepsafezone 0` — and even then an edge or two can survive. The backdrop is
  therefore deliberately **overscanned**: `position -0.02 -0.02`, `size 1.04 1.04`. Symmetric, so
  `halign/valign center_ref` on the child dialog still centres it on screen.
- The HUD (party nametags, POI markers) **draws over it** unless it claims a high `priority`.

**Focus accounting is the trap here.** Input focus is an additive counter:
`SimulateDeath` → `LockControls(true)` (+1/device), the menu's `super.OnShow()` → `LockControls()`
(+1/device), `super.OnHide()` (−1/device), `EnterSpectate()`'s single explicit release (−1/device) —
net zero. The death menu therefore adds **no** focus calls of its own. Note `LeaderboardMenu` *does*
add `ChangeGameFocus(1)` / `ResetGameFocus()` on top of `super`; do not copy that pattern, since
`ResetGameFocus` zeroes the counter rather than decrementing it.

## The server driver and the client side

Killer attribution goes through `BattleRoyaleKillAttribution` — see [kill-attribution.md](kill-attribution.md).
`BattleRoyaleSpectators.ResolveKillerUid` is now a one-line delegate to it. `RecordDeath` is
**first-write-wins**, which is what makes the documented double `RemovePlayer` and the
unconscious-disconnect path harmless; `RecordDeathWithKillerUid` is its general form, because a
killer known only as a uid may have no `PlayerBase` left at all.

One driver: `BattleRoyaleSpectators.Tick()`, from the existing 10 Hz block in
`BattleRoyaleServer.Update()`. Its four passes are liveness sweep (the *primary* disconnect
detector — `PlayerDisconnected` is unreliable for a client controlling no entity), deferred entry,
target liveness (the catch-all), and a 1 Hz keepalive push.

Which states allow it is a `BattleRoyaleState.AllowsSpectate()` virtual, default `false`, overridden
in `6_BattleRoyaleRound`, `7_BattleRoyaleLastRound` and `5_BattleRoyaleStartMatch` (on `b_IsGameplay`).

`BeginSpectate` calls **`SelectPlayer(identity, NULL)` before `SelectSpectator`**, dropping the
corpse from the connection's selection. Only the *selection* goes; the corpse itself is never
deleted, so it stays in the world, replicated and lootable. What actually keeps the watched player
replicated is the camera's `UpdateSpectatorPosition` pushes — see the top of this section, and do not
re-derive the "bubble is pinned to the selected object" theory from the shape of this call.

Client side, **`GetGame().GetPlayer()` does NOT go NULL while spectating** — it keeps returning the
**corpse**, a perfectly valid `PlayerBase` that simply is no longer simulated. (Measured: the tint
trace reads `spectating=1, player=1`.) This was assumed the other way round for most of the feature's
life and it is the trap here, because it makes "is there a player object" a silently wrong test for
"is this an ordinary living client" — no error, no log line, just an effect queued onto a corpse
whose `m_ProcessAddGlassesEffects` nothing ever drains. **Ask `BattleRoyaleClient.IsSpectating()`**;
`GetReferencePosition()`, `GetSubjectPosition()` and `ApplyOutOfZoneTint()` all do.

`BattleRoyaleClient` routes position reads through `GetReferencePosition()`, and
`EnterSpectate()` undoes what `SimulateDeath`
did — the death screen, the five zeroed sound buses, the additive input-focus lock (exactly one
release, behind a latch) and the vanilla HUD. It also calls
`PPEManagerStatic.GetPPEManager().StopAllEffects(PPERequesterCategory.GAMEPLAY_EFFECTS)`: dying
*while unconscious* leaves `PPERequester_UnconEffects` running forever, because `CommandHandler` only
reaches `OnUnconsciousStop()` through an `IsAlive()` branch a corpse never takes — that was a black
halo around the spectator's screen for the whole match. `GAMEPLAY_EFFECTS` rather than `ALL`, so the
ESC-menu and spawn-selection blur survive. The paired half is in `ShowDeadScreen`, which writes full
`m_CurrentShock` on the dying body so `ShockHandler` stops re-asserting tunnel vision every frame,
and clears `"UnconsciousAttenuation"` so the spectator does not hear the match muffled. `BattleRoyaleSpectatorCamera`
(`Scripts/Client/4_World/BattleRoyale/`, **unguarded**, like vanilla's `dayzspectator.c`) anchors on
the target's **head bone** rather than a stance offset, and never reads the target's aim angle —
`GetCommandModifier_Weapons()` can be NULL for a remote entity, which would silently flatten the
camera.
