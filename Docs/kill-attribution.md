# Kill attribution

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

**One resolver answers "whose kill was this": `BattleRoyaleKillAttribution`
(`Scripts/Server/4_World/Entities/`).** Four consumers used to derive it independently and disagreed
— the spectator killer chain, the old `player.kill` webhook (removed), the kill credit, and
`PlayerBase.EEHitBy`'s `last_unconscious_source`. Two facts drive it, and both have caused bugs:

- **`EEKilled`'s `source` is the WEAPON** for every gun and melee kill. `ResolvePlayerSource` does
  the hierarchy-parent step, with the `EntityAI.Cast` **null-checked** — a source that is not an
  `EntityAI` (a building, a vehicle part) used to dereference NULL at two separate call sites.
- **For an explosive or a trap the source is the DEVICE**, which has no hierarchy parent once armed.
  The responsible player is knowable only because the device recorded them at arm time, as a **plain
  string** — which is exactly what lets a kill outlive its owner's death *and* their disconnect.

**Devices are modded at the vanilla PARENTS, `ExplosivesBase` and `TrapBase`.** Naming the leaves
(`Grenade_Base`, `LandMineTrap`) meant Claymore, IED and Plastic Explosive — the archetypal traps —
credited their classname instead of their owner. `ExplosivesBase.OnPlacementComplete` is the one hook
that covers arming as well as placement: `ActionArmExplosive.OnFinishProgressServer` calls
`OnPlacementComplete(action_data.m_Player, …)`. `Grenade_Base` therefore carries **only `OnUnpin`**;
re-declaring `m_ActivatorId` on it would shadow the parent's. `ActionTriggerRemotely` re-attributes
to whoever pressed the detonator, and **sets the activator before `super`**, since `super` is what
detonates.

⚠️ **The `EEKilled(Object killer)` override on a device cannot use a bare `PlayerBase` cast** — the
same weapon-not-shooter rule applies, so the cast this replaced could never succeed and shooting an
armed charge silently kept crediting whoever placed it.

⚠️ **`OnPlacementComplete` must be hooked on the PARENT; `OnActivatedByItem` must be hooked on the
LEAF. The two sit inches apart and behave oppositely.** Vanilla's four explosives — `Grenade_Base`,
`ClaymoreMine`, `ImprovisedExplosive`, `Plastic_Explosive` — **none** of which calls
`super.OnActivatedByItem`, so an override of it on `ExplosivesBase` is dead code for every explosive
in the game. It was written there first and looked right. The symptom is a device that records its
owner correctly and still credits `<environment>`.

**A grenade rigged to a tripwire is the case that needs it**, and it is the one real path where the
device that KILLS is not the device that knew the owner: `TripwireTrap.SetInactive` calls
`attachment.OnActivatedByItem(this)` and then **drops** the attachment, so the grenade does the
damage while the trap holds the activator. Vanilla's `Grenade_Base.OnActivatedByItem` answers by
calling `Unpin()`, which does reach the mod's `OnUnpin` — but the grenade's hierarchy root is the
*trap* by then, not a player, so it resolves nobody. `BR_InheritActivatorFrom` copies the activator
across, called before `super` because `super` starts the fuse. Same handoff applies to an IED with
grenades attached (`ImprovisedExplosive.OnActivatedByItem` activates its own attachments).

**Kill credit lives in `BattleRoyaleKillLedger`, a uid → kills map, and `br_kills` is now a mirror
of it.** The explosive branch of `0_BattleRoyaleState.OnPlayerKilled` used to write the webhook JSON and then
fall out of the `if`, so **no grenade or mine kill had ever scored** — not on the HUD counter, the
admin spectator tags, or the ladder. Both branches funnel through `CreditKill` now, which is a no-op
on an empty uid. A **corpse** still resolves through `FindPlayerByUid` (the mod never deletes bodies),
so a dead-but-connected killer watches their own counter tick.

**A posthumous kill has to AMEND the ladder, not add to it.** `RecordExit` fires and locks the uid
into `m_Recorded` the moment its owner leaves the match — which is before their grenade lands.
`BattleRoyaleLeaderboard.AmendKills` recomputes `MatchPoints` against the stored
`BattleRoyaleMatchRecord` (board, rank, ranked, kills, points) and applies the **deltas**; entries are
cumulative across matches, so re-adding would double-pay. A uid with no match record needs nothing —
`RecordExit` reads the ledger itself and gets the right answer.

**The `AddPlayerKill` RPC now carries the authoritative TOTAL, not an increment.** The client handler
ignored its payload and did a local `+= 1`, which drifts the moment a kill is scored while that client
controls no entity — a spectating killer whose grenade landed after they died.

⚠️ **`EnScript.GetClassVar` DOES reach a var declared on a modded PARENT class — measured 2026-08-12
— but its return code is meaningless.** This matters because `Extra/KillFeed` reads the activator
reflectively (the discipline rule forbids it naming a BattleRoyale symbol), and those vars now live on
`ExplosivesBase` rather than on `Grenade_Base`. A round-trip through a `Grenade_Base` instance
returned `PROBE-OK`, so the seam is sound. The return code was **0 for all four cases**: a var
declared directly on the class, a var that exists nowhere, a successful `SetClassVar`, and a
successful `GetClassVar`. Never branch on it — the same trap as `DiagMenu.BindCallback`. An empty
string back is *not* evidence of failure, which is why the first probe (a bare read of a fresh
grenade) was ambiguous and had to be redone as a round-trip.
