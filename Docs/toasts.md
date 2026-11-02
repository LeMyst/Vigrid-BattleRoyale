# Notifications (toasts)

> Split out of the root [`CLAUDE.md`](../CLAUDE.md), which carries the build, test and EnfusionScript rules every change needs - read that first. A correction made here must also reach the memories and skills that repeat the rule (see *Workflow* in `CLAUDE.md`).

Every player-facing message from this mod is a **toast**: `BattleRoyaleToasts` (`Scripts/Client/5_Mission/GUI/HUD/`) plus `GUI/layouts/hud/toasts.layout` and `toast_row.layout`. It replaced `ExpansionNotification`, which was the last unguarded reason the client needed Expansion loaded.

**The wire did not change.** The server still ships a bare stringtable key over the existing `NotificationMessage` RPC and the client localises it on arrival — see *Localization* in the root [`CLAUDE.md`](../CLAUDE.md), which is unaffected. Two producers reach the widget and they hand over different things on purpose: the RPC path arrives **already localised** (the `%1..%5` substitution and the `READY_KEY` / `UNSTUCK_KEY` tokens must happen first), while `BattleRoyaleClient.NotifyLocal` passes a `#key` untouched and lets the widget resolve it.

Five things are load-bearing, and each was a bug first:

- ⚠️ **`"size to text v"` MUST STAY 0.** On a wrapping widget it reports height **zero** *and* overrides an explicit `SetSize`, so the row ends up `W×0` and draws an empty plate with no text. Vanilla never pairs it with `wrap` either.
- **`GetTextSize()` already returns the WRAPPED size** — width of the longest line, full block height (measured `301×66` for three 22 px lines at a 528 px wrap). So the height comes straight off it; **do not divide by the wrap width to count lines**, that double-counts and only shows up on the first message long enough to wrap twice.
- **`RichTextWidget.GetContentHeight()` is the obvious API and has ZERO call sites in `P:\scripts`** — the shape that compiles, runs and silently does nothing. Not used.
- **The pool binds by TOAST ID, not by a flag on the model.** A new toast inserts at index 0 and shifts every existing model onto a different widget, so a per-model "already bound" flag skips `SetText` on the widget that just changed content.
- ⚠️ **`BR_TOAST_TOP_PX` is in REFERENCE pixels and is scaled by `screen_w / 1920`**, because the compass strip it clears scales the same way. Held at flat pixels the gap between them changed with the resolution (68 / 54 / 40 px at 1280 / 1920 / 2560) and looked right only where it was eyeballed.

Diag: **HUD & Menus → Push Toasts** raises a random 1-3 from a pool of uneven lengths. The varying count is the point — a burst landing on top of live toasts is what exposed the pooling bug above.
