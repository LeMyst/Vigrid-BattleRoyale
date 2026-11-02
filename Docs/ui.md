# UI: lobby name tags, scrolling lists and the trimmed HUD

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

## Lobby name tags

Every living non-teammate wears their name over their head while the players are still gathered
before the match — `BattleRoyaleLobbyTags.c` plus `lobby_tags.layout` / `lobby_tag.layout`, driven
each frame from `BattleRoyaleClient.Update`.

**It replaced a "point at somebody to read their name" tag, and that tag was vanilla's own.**
`IngameHud.RefreshPlayerTags` / `ShowPlayerTag` ship `#ifdef PLATFORM_PS4` and never run on PC; the
mod's `modded class IngameHud` used to call them directly whenever `!match_started`, and repaint the
text with the resolved name. It names one player at a time, chosen by a 25 m raycast down the camera
axis, and draws it at a fixed spot beside the crosshair rather than over the character.

Four things worth keeping:

- ⚠️ **`!match_started` was the wrong phase test, and this is a general trap.** `match_started` is a
  one-way latch set by the `StartMatch` **broadcast**, so a client that connects *after* that
  broadcast never receives it and reads `false` for its whole session. In practice that is always an
  admin — everyone else is kicked — which is exactly how the lobby-only tag ended up live for an
  admin spectating a running match. The replacement is `BattleRoyaleRPC.lobby_phase`, a discrete fact
  pushed **on every state transition** (one call in `BattleRoyaleServer.Update`, so no state has to
  remember) **and per-identity in `PlayerLoadedIn`**, which is what a late joiner needs. Any
  broadcast-latched flag has this bug latent in it; check for a per-connect seed before trusting one.
- The window is `BattleRoyaleServer.IsLobbyPhase()` — the current state casting to
  `BattleRoyaleDebugState`, which covers the lobby *and* the pre-match countdown and nothing else.
  Deliberately narrower than `!match_started` was: spawn selection and the drop are outside it, so no
  name hangs over anybody at their spawn point.
- **Positions are local, names come over `SetLobbyNames` keyed by network id.** In the lobby everyone
  is in the same clearing and therefore inside the bubble, so `ClientData.m_PlayerBaseList` gives
  exact per-frame positions; the server sends who to name and what to call them, with the
  recipient's own teammates already removed, so no party composition and no SteamID64s go on the wire.
- ⚠️ **`PlayerBase.GetIdentity()` IS populated client-side for a remote player — measured
  2026-08-12, `noidentity=0` sustained.** It is written down because the opposite was asserted
  confidently enough to cause a rewrite: when this overlay was first reported broken, the theory was
  that remote identities are null client-side and that `VigridPartyAPI.FindLocalPlayer` and
  `BattleRoyaleSpectatorTags` only appear to work because each falls back to a server-pushed
  position. That reasoning is tidy and **wrong**. The overlay had never been broken at all — the
  report was about a different tag. The network-id transport that replaced it works and is what
  ships, but it is not *needed*; the client-side version is strictly smaller. Do not re-derive the
  identity claim from the shape of the code.
- **Party members are excluded server-side**, before the packet is built — they already carry the
  party's own coloured tags, and two labels over one character is the stacking that had to be fixed
  once already for the admin overlay. Doing it there means `BattleRoyaleLobbyTags` names no Party
  symbol at all.
- **The funnel diagnostic earned its place and is kept.** `[Lobby] tags rows=… noentity=… noidentity=…
  far=… offscreen=… drawn=…`, 2 s apart while the overlay is active. It is what turned a three-way
  argument into a one-line answer, and it is what should be reached for first next time.

`IngameHud.BR_SuppressPointTag` now hides the point tag whenever the mod draws its own names — in the
lobby, and for an admin spectator. **It hides the root `m_PlayerTag`**, which is what makes it work
against DayZ Expansion's NameTags addon too: Expansion's own `modded IngameHud` drives the same
vanilla-owned root and parents its icon inside it, and neither vanilla nor Expansion ever calls
`Show(true)` on that root — they fade the text alpha. So the hide wins whichever order the two modded
`Update`s run in, which matters because nothing in `requiredAddons` pins that order down.

## Scrolling lists (leaderboard, party menu)

**The two scrolling lists — `LeaderboardMenu` and `VigridPartyMenu` — share one construct, and all three of its parts are needed.** A `ScrollWidget` owns its child's geometry, so each wraps a `WrapSpacer` carrying `"Size To Content V"` (plus `"Scrollbar V"` on the scroll itself); the script must therefore **never `SetPos` a row**, and must **`Unlink()` surplus rows rather than hide them** — a spacer lays out the children it *has*, so a hidden row keeps its slot and leaves dead scroll below a shorter refresh. Both also implement `OnMouseWheel` → `VScrollStep`, following vanilla's own `ScrollBarContainer`. Note this binds only under a spacer: `VigridPartyHud`, `BattleRoyaleSpeakingList` and `Extra/KillFeed`'s row pools free-position with `SetPos` into plain panels and correctly keep hiding their surplus.

⚠️ **A list that "does not scroll" is usually a list that fits.** The leaderboard was diagnosed twice as a widget bug when its fake data was 12 rows against a ~16-row viewport — content height and scrollbar state were both already correct. The diag fixtures are deliberately oversized for this reason (`BRDiagFillBoard` at 40 solo / 25 group, `Fake Online Players` defaulting to 20 of a possible 60): **a fixture that fits cannot reach the feature it exists to exercise.** Count rows against the viewport before changing widget code.

⚠️ **`LeaderboardMenu` pools rows across THREE tabs but only TWO row layouts.** The last-match table uses `match_summary_row.layout`, so `SwitchBoard` must `TrimPool(0)` before repainting — a widget built for one layout and reused for the other answers NULL to every `FindAnyWidget`, which paints a **silently blank table** rather than erroring. The original two-tab code had no such hazard because both its tabs shared one layout.

⚠️ **A grouping panel with the default `ignorepointer 0` EATS CLICKS on everything under it.** The two panels that swap the leaderboard's tab bodies are full dialog size and declared *after* the tab buttons, so Solo / Group / Last Match silently stopped responding — no error, no log line — while `CloseButton`, declared after the panels, kept working. That asymmetry is the fingerprint. **`ignorepointer` does NOT propagate to children**: vanilla's `day_z_respawn_dialogue.layout` stacks it on three nested parents and its `ButtonWidget`s inside them work fine, so a pure grouping node should always carry it. Note the diagnosis came from the *server* log — a `RequestLeaderboard` retry firing once a second while still on the solo board proved `Update()` was running and the board never changing. `LeaderboardMenu.OnClick` now traces the clicked widget name, as `DeathScreenMenu` already did.

## The trimmed vanilla HUD

**The vanilla right-hand HUD is trimmed.** `modded class IngameHud` (`Scripts/Client/5_Mission/GUI/IngameHud.c`) hides the thirst, hunger and temperature notifiers plus the `NotifierDivider` beside Blood, and shifts `BadgesSpacer` / `BadgesPanel` right to close the resulting gap. `Extra/PreventPlayerModifiers/` already makes `ThirstMdfr.OnTick` and `HungerMdfr.OnTick` return immediately, so those three icons never move for a whole match — they are pinned decoration. Gated on `BR_HIDE_SURVIVAL_NOTIFIERS` (`BattleRoyaleConstants.c`), compile-time because the settings files are server-side only and this is a client cosmetic.

Three things about that hook are load-bearing:

- **`Show(false)` targets the parent panel (`Thirsty`, `Hungry`, `Temperature`), never the `Icon<Name>` image inside it.** Vanilla's `InitBadgesAndNotifiers` unconditionally `Show(true)`s every `Icon*` widget and `DisplayTendency` keeps tinting them; a hidden *parent* is not drawn whatever happens to its children, so nothing has to be re-asserted per frame. `NotifierDivider` is the one widget here that no vanilla script references at all — only `BadgeNotifierDivider` is managed, by `IngameHudVisibility`'s `NO_BADGE` flag.
- **The hook is `InitBadgesAndNotifiers()`, not `Init()`.** `Init()` calls it once at startup and `respawndialogue.c` calls it again after a respawn, so one override covers both. That is also why the badge reposition uses **absolute** x (`BR_HUD_BADGES_SPACER_X` / `_PANEL_X`) rather than a delta — a second pass must be a no-op.
- Everything under `HudPanel` is `halign right_ref`, so `position x` is the distance from the parent's **right** edge and hiding a middle widget leaves a hole rather than reflowing. Vanilla's right-to-left order is `Health` 0, `Blood` 43, `NotifierDivider` 86, `Temperature` 96, `Hungry` 139, `Thirsty` 182, `BadgesSpacer` 213, `BadgesPanel` 252. The badge group moves right by 143 so its divider lands on the old 86.

**The injured-leg badge is suppressed too, and a BADGE needs a different seam from a notifier.** `BR_HIDE_INJURED_LEGS_BADGE` covers it, and the override is `DisplayBadge( int key, int value )` — forcing `value = 0` for `NTFKEY_LEGS` and passing the call to `super`. Four things make it the shape it is:

- **A `Show(false)` in `InitBadgesAndNotifiers` cannot work here**, which is the trap, because it is exactly what the three notifiers beside it do. Those are static widgets; badges are dynamic. Vanilla's `DisplayBadge` re-walks *every* badge on *every* call and `Show(true)`s any whose stored value is positive, so a one-time hide is silently undone by the first fall that scratches a leg.
- **`super` must still run.** `DisplayBadge` is also what maintains `EHudContextFlags.NO_BADGE`, the only thing driving `BadgeNotifierDivider` (`IngameHudVisibility`'s element link map). Suppress the badge by any route that skips `super` and that divider stands beside a badge nobody can see. Forcing the value to 0 rather than dropping the call also keeps `m_BadgesWidgetDisplay` honest.
- **No layout work, unlike the notifiers.** `BadgesPanel` is a `WrapSpacerWidgetClass` (`day_z_hud.layout:514`), so a hidden child collapses and the siblings reflow — the opposite of the `halign right_ref` hole described above, and why `BR_HUD_BADGES_*` are untouched.
- ⚠️ **"Just prevent the leg damage instead" is the wrong fix and was considered.** Leg *health* is not leg *breakage*: `InjuredLegNotfr` tiers `1 - min(GetHealth01("LeftLeg"), GetHealth01("RightLeg"))` at 0.05/0.35/0.65 and only consults `GetBrokenLegs()` to bail out, so the badge lights at a 5% scratch even though `Extra/PreventPlayerModifiers` makes a fracture impossible. Blocking that damage would need a config patch to the leg zone, whose `transferToGlobalCoef=0.25` (`P:\dz\characters\data\config.cpp:787`) is **how a bullet in the leg hurts you at all** — it would turn legs into bullet sponges. `DamageAllLegs()` is only the *extra* leg damage from falls, car exits, landmines and bear traps; ordinary gunfire damages the zone natively, so emptying it removes the harmless sources and keeps the common one. And leg health drives nothing else in this build: the limping override reads it only under the `BROKEN_LEGS` force mask (`injuryhandler.c:137-148`), which is unreachable here. The badge is its only consumer, so hiding the badge is the complete fix, not a cosmetic patch over a real one.
- The separate `Fracture` badge (`NTFKEY_FRACTURE`) is deliberately left alone — it is the one that matters again the moment `Extra/PreventPlayerModifiers` is disabled, and it costs nothing while that addon is in the build.
