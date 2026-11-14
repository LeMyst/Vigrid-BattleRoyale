# Death recap and the last-match summary

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

Two halves of one feature. On death the player gets a one-line recap; back in the **lobby of the next
match** they get a full card and the previous match's standings, on a third tab of the F4 leaderboard.

**The summary is persisted and read in the next lobby rather than shown at match end, and that is the
whole design.** The server process restarts between matches, so everybody — including the winner, who
`KickWinner` drops 15 s after they win — reconnects into a fresh lobby. It is the only place a summary
can reach every player, it makes the death-screen change a single line, and it gives reconnect
resilience for free (the card is keyed by uid out of a file, so dropping mid-match and rejoining still
finds you). That last property is why the uid must stay in the file.

**One resolver answers the recap: `BattleRoyaleKillAttribution.ResolveKillDetails`**, beside the
existing `ResolveKillerUid` and for the same reason that class exists — weapon-and-range already had
*two* independent derivations in the tree (`Extra/KillFeed/KillFeedDeath.c` and the webhook JSON block
in `0_BattleRoyaleState.OnPlayerKilled`), and a third inline copy would repeat the original mistake. It
is 4_World because `PlayerBase` and `BattleRoyaleMatchStats` both need it. The webhook's copy is gone:
the stats API now takes kills and causes from the `last_match.json` rows in its `finish` call.

`BattleRoyaleKillCause` (2_GameLib, unguarded) is **BR-owned and numerically independent of
`KillFeedCause`** — that addon is optional by contract and can be deleted from the build. Do not
"align" the two. The zone is the one environmental cause that can be named, and only because the two
`DecreaseHealthCoef` sites drop a timestamp that `ConsumeZoneHint` **consumes**; a hint left behind
would mislabel the player's next environmental death.

**`BattleRoyaleDeathRecord` lives in 4_World**, not beside `BattleRoyaleSpectators` which owns and
writes it, because `BattleRoyaleMatchStats.RecordExit` copies the recap onto the summary row and a
4_World method cannot name a 5_Mission type. Every recap field is written **inside `RecordDeath`'s
existing first-write-wins guard** — the unconscious-disconnect path fires a second `EEKilled` whose
source is the *victim*, so a second pass would overwrite good attribution with `ENVIRONMENT`.

## Damage: measured as a delta, and the two bugs that took real matches to find

`TotalDamageResult` reports **computed** damage, not applied, and is getter-only. Damage is therefore
measured as the health/blood the hit actually removed, latched in `EEOnDamageCalculated` (which runs
first) and subtracted in `EEHitBy` after its `super`. **Both pools**, `Math.Max` of the two fractions
normalised by the victim's own maxima: DayZ death is `Health <= 0` **or** `Blood <= 0` and a firearm
body shot is mostly a *blood* hit, so health alone under-reports every gunfight and reports **zero**
for a bleed-out kill. The unit is "fraction of a player removed", so one full-health kill scores
exactly 100 — which is also the arithmetic that catches under-counting.

⚠️ **The guard must be "were they alive BEFORE this hit", not `IsAlive()`.** Vanilla's `IsAlive()` is
`!IsDamageDestroyed()` — purely health-based — and `EEHitBy` runs *after* the engine applies damage,
so a killing blow reads as "already dead" and the one hit that decided the fight is discarded. Caught
because a knife kill on a full-health player scored **76**: the per-hit deltas cannot sum below the
health actually lost. The pre-hit latch answers it exactly, and still refuses corpse-shooting.

⚠️ **Pair the latch to its hit with a CONSUMED FLAG, never a clock comparison.** This was
`br_prehit_ms != GetGame().GetTime()` — millisecond-exact equality — and it dropped hits at random,
because `GetTime()` is live ms and the engine applies damage, processes bleeding and runs the shock
transfer between the two hooks. The same player killing the same opponent with the same weapon scored
**100 in one match and 0 in the next**. The 200 ms age check that remains is only a safety net for the
orphan case (another mod cancels the damage, so nobody consumes the latch); "no latch at all" is a
**separate** rejection reason, because it is a different fault with a different fix.

**Known gap: bleed-out damage is credited to nobody**, since blood loss drains through the bleeding
manager rather than through `EEHitBy` with a player source.

## Persistence: ONE write

`$profile:Vigrid-BattleRoyale\last_match.json`, written **once**, in `9_BattleRoyaleRestart.Activate()`.

The tempting alternative — the leaderboard's debounced flush — is wrong here, and not for performance
reasons: it would make the file do two jobs with opposite lifetimes. A partial mid-match write holds
players whose `br_position` is *groups remaining right now* rather than a finishing place, so every
survivor reads as 4th, and the same flush cycle overwrites the backup, losing the previous match too.
The ladder can afford that trade because it is cumulative across months; this is one cosmetic match.
Writing once also makes the in-memory `m_Previous` immutable for the process lifetime — which is what
makes serving it safe with no sequence number — and turns the copy-aside into a free one-deep history.

⚠️ **Not in `8_BattleRoyaleWin`.** The winner's `RecordExit` does not run until `KickWinner`, 15 s
*after* that state activates, so a write there produces a file with no placement and no survival time
for the winner — the one row everybody looks at first.

**Sorted by place on write AND on load.** `RecordExit` appends in death order, so unsorted the table
renders the winner underneath the loser. The write-time sort cannot repair a file produced by an older
build or written by hand, and everything read off that disk is untrusted anyway. The winner is read
*before* the sort, because it is the last **exit**, not the best place.

**I/O profile: nothing touches the disk while anybody is playing.** `JsonFileLoader` is synchronous —
EnfusionScript has no async I/O — so every save blocks the main thread, which is what makes *when*
matter more than how big. The load is one small file (~11 KB at 40 rows, ~18 KB at the 64 cap) during
lobby entry; the save is after the match has ended, 10 s ahead of `RequestExit`. Per hit the ledger is
pure in-memory map work, and its per-hit trace is gated on `CheckLogLevel` so the string is not even
built at production log levels. For contrast, the thing that *does* write mid-match is the
pre-existing `BattleRoyaleLeaderboard` flush every `BR_LEADERBOARD_FLUSH_DEBOUNCE_MS` (15 s),
serialising up to 5000 entries — that is the one to suspect if periodic hitches ever appear.

## The wire

`RequestLastMatch` (no payload, actor from `sender`, its **own** cooldown map — sharing the
leaderboard's means a fast tab switch is silently refused) answered per identity by
`SetLastMatchTable` + `SetLastMatchRecap`. **No SteamID64 on any of it**: `self_index` identifies the
local row, and is `-1` for anyone who did not play — the *common* case in a lobby, which must hide the
card rather than paint a zeroed one.

⚠️ **Never call `RequestLeaderboard` with `BR_LEADERBOARD_BOARD_LASTMATCH`.** `ServeRequest` treats
anything that is not `GROUP` as `SOLO`, so board 2 is answered with the solo ladder tagged as solo
while consuming the requester's cooldown — the tab just looks dead.

`grouped` comes from `VigridPartyAPI.IsReady()`, never from `#ifdef VIGRID_PARTY`, or a server with
the party manager disabled labels a player count as a squad count (#158 again). The squad block is
summed **client-side** from the table, which is only sound while the table is complete — hence the
`TRUNCATED` flag, on which the client hides the block rather than showing a quietly wrong figure.

`SetLastMatchRecap` has **two send points and one handler** — pushed at death, and again when the
lobby asks. They cannot be confused within a session because a session spans exactly one server
process; the one exception is an admin, who is exempt from the late-join kick and can see match N's
recap under match N−1's header.

## Testing it

`DIAG_DEVELOPER` entries under *HUD & Menus*: **Fake Last Match**, **Recap Cause**, **Did Not Play**
and **Open Death Screen** — the last of which must set `suppress_alive_close`, because offline the
player is always alive and `DeathScreenMenu.Tick()` closes on `IsAlive()`, which reads as a broken
layout.

⚠️ **Build the fixture so it can FAIL.** The 40-row fixture was written in rank order — already sorted
by construction — so it could never have caught the missing sort; a real two-player match did, on the
first try. This is the same trap as the leaderboard fixture that fitted its viewport and so could not
test scrolling. Whatever property is under test, construct the fixture to violate it.

⚠️ **Log every rejection, with its own reason.** A guard that only logs on success cannot distinguish
"nothing arrived" from "everything arrived and was thrown away" — both are no line at all. A damage
total of zero was ambiguous for two full test matches until one build logged each early `return`; it
named the guilty guard on the first hit.
