# Random Menu Gear

Re-dresses the main-menu intro character in a random outfit, with a slung rifle and a melee weapon,
re-rolled every time the menu is shown. Purely cosmetic — it makes the menu look like the mod rather
than like a default DayZ install.

|                 |                                                        |
|-----------------|--------------------------------------------------------|
| **PBO**         | `extra_randommenugear.pbo`                              |
| **Side**        | client-local — no guards; gated at runtime by `IsDedicatedServer()` |
| **Stages**      | `4_World`, `5_Mission`                                  |
| **`defines[]`** | `RANDOM_MENU_GEAR` — declared for consistency, nothing consumes it today |
| **Standalone**  | yes — no `BattleRoyale*` symbol referenced              |

No assets, no layouts, no stringtable, no settings, no RPC. Nothing outside the addon calls it.

## What it is not

**This is not a fix for the broken character save that makes the menu character render naked.** It only
decorates whatever object was spawned. If your menu character appears nude without this addon, that is
a separate problem and this does not address the cause.

## How it works

`RandomMenuGear.Apply(Man player)` clears every managed slot and re-rolls it. It is safe to call
repeatedly. Two hooks call it, and both are needed:

| Hook | Covers |
|---|---|
| `IntroSceneCharacter.CreateNewCharacterById` | every character *creation* — initial menu load, the prev/next arrows, returning from character creation |
| `MainMenu.OnShow` | simply re-showing the menu, which calls `OnChangeCharacter(false)` and never recreates the character |

Per slot, `Apply` finds any existing attachment and deletes it (clearing leftovers from both the
character save and the previous roll), picks a random entry from that slot's pool, and attaches it with
`GameInventory.CreateAttachmentEx`.

Twelve slots, ordered so nothing is attached before the layer under it:

`BODY`, `LEGS`, `FEET`, `VEST`, `BACK`, `HEADGEAR`, `MASK`, `EYEWEAR`, `GLOVES`, `ARMBAND`,
`SHOULDER` (9 rifles), `MELEE`.

An **empty-string entry in a pool means "leave this slot bare"** — the same trick vanilla's
`GenerateRandomEquip` uses. `MASK` carries two empty entries so a face-covering mask is the exception
rather than the rule; `VEST`, `EYEWEAR` and `ARMBAND` carry one each.

## The load-bearing design decision

Gear is applied straight to the spawned object and is **deliberately never written into
`MenuDefaultCharacterData`**. That map is serialized to the server on connect and saved locally, so
writing to it would leak random menu gear into the player's real spawn loadout.

If you extend this addon, keep that rule.

## Notes

- Pool construction is **lazily initialised** on first use, not at static-initializer time:
  `InventorySlots` constants are populated by the engine from `CfgSlots`, so they are only safe to read
  once the game is up.
- A rifle renders slung because `Rifle_Base` declares `inventorySlot[] = {"Shoulder","Melee"}`, which
  makes it a proper slung proxy on the static menu idle pose.
- This addon's `modded class MainMenu` chains cleanly onto the mod's own `modded class MainMenu`,
  because that one does not override `OnShow()`.
- The `CfgPatches` class is `Random_Menu_Gear` (underscored) so the config class never collides with
  the script class `RandomMenuGear`. The PBO name comes from the folder, not from that class.

## Changing the gear

The pools are hardcoded parallel arrays in `BuildPools()` (`Scripts/4_World/RandomMenuGear.c`). Add or
remove classnames there; every classname currently in the file was verified against the vanilla
configs.

## Disabling

Rename `config.cpp` → `config.cpp.disabled` and rebuild; the folder is then skipped entirely and the
menu character reverts to vanilla behaviour.
