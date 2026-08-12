# Change Feedback URL

Repoints the vanilla **Feedback** button away from Bohemia's bug tracker and at this mod's GitHub
repository, so a player reporting a problem lands somewhere the mod's maintainers actually read.

|                 |                                                              |
|-----------------|--------------------------------------------------------------|
| **PBO**         | `extra_changefeedbackurl.pbo`                                 |
| **Side**        | client-facing, but unguarded — it compiles on the server too  |
| **Stages**      | `5_Mission`                                                   |
| **`defines[]`** | none                                                          |
| **Standalone**  | **no** — depends on the main mod (see Caveats)                |

## How it works

DayZ has three separate Feedback buttons, each with its own handler. All three are overridden with the
same one-line body, `GetGame().OpenURL( GITHUB_URL );`:

| File | Class | Method | Vanilla URL it replaces |
|---|---|---|---|
| `Scripts/5_Mission/GUI/InGameMenu.c` | `InGameMenu` | `OpenFeedback()` | `https://report.bistudio.com/projects/dayz` |
| `Scripts/5_Mission/GUI/NewUI/MainMenu/MainMenu.c` | `MainMenu` | `OpenFeedback()` | `https://report.bistudio.com/projects/dayz` |
| `Scripts/5_Mission/GUI/NewUI/MainMenu/MainMenuNewsFeed.c` | `MainMenuNewsfeed` | `OpenFeedback()` | `https://feedback.bistudio.com/tag/dayz` |

None of the three calls `super`, so the vanilla URL is replaced outright rather than opening both.

## Changing the URL

The destination is **not** defined here. It comes from the main mod:

```c
// Scripts/Client/2_GameLib/BattleRoyaleConstants.c:24
static const string GITHUB_URL = "https://github.com/LeMyst/Vigrid-BattleRoyale";
```

Edit it there and every Feedback button follows.

## Caveats

- **Not standalone.** `requiredAddons[]` lists only `DZ_Scripts`, yet the code reads `GITHUB_URL`,
  a symbol owned by `BattleRoyale_Scripts_Client`. It compiles today only because `2_GameLib` is an
  earlier stage than `5_Mission` — this PBO will not build or run without the main mod. Either add
  `BattleRoyale_Scripts_Client` to `requiredAddons[]`, or inline the URL, before reusing it elsewhere.
- The main mod also declares `modded class MainMenu` and `modded class InGameMenu`, and
  `Extra/RandomMenuGear` declares a third `modded class MainMenu`. These chain rather than conflict —
  none of the others touches `OpenFeedback`.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely and the
vanilla Bohemia links come back.
