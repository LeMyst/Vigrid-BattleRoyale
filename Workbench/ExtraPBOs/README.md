# Workbench/ExtraPBOs

Prebuilt PBOs, **written by other people**, that `CI0_CopyExtraPBO.bat` drops into
`%ModBuildDirectory%@Vigrid-BattleRoyale\addons\` (and their `.bikey`s into `keys\`) at the end of
a build, so the server the maintainer runs carries them without the operator installing separate
mods.

Nothing in this folder is built from this repo, and nothing in it is tracked by git — `.gitignore`
is `*` with only this README and itself excepted.

## The copy is opt-in and off by default

`CI0_CopyExtraPBO.bat` skips the copy unless **`CopyExtraPBOs=1`** is set in the gitignored
`Workbench\user.cfg`. That default is what makes the build reproducible: because the folder is
untracked, a clone has an *empty* one, so an unconditional copy meant two machines building the
same commit produced two different mods, and the difference was invisible — the extra PBOs load
exactly like the mod's own.

`Build.log` records which branch ran, so the built artifact always says what went into it.

## What is currently here

| File | Author | What it does |
|---|---|---|
| `ReduceCarDamage.pbo` + `.daemonforge.bisign`, `daemonforge.bikey` | DaemonForge | `modded class` under `ReduceCarDamage/scripts/4_world` reducing vehicle damage. Ships an **Apache License 2.0** text inside the PBO. |
| `slowzombies.pbo` + `.munghard.bisign`, `Munghard.bikey` | Munghard | Config-only patch of `DZ_Characters_Zombies` — `CfgAIBehaviours` / `CfgNoises` movement and alert tuning. No licence text included. Built 2019-06-28. |

## ⚠️ Before turning this on for a public release

Copying these files into `@Vigrid-BattleRoyale` **redistributes another author's work under this
mod's name**, together with their signing keys. That is a licensing question, not a build question:

- `ReduceCarDamage` carries Apache-2.0, which permits redistribution provided the licence and
  attribution travel with it — so the PBO's own embedded `LICENSE` must not be stripped.
- `slowzombies` ships **no licence at all**, so no redistribution right can be assumed from the
  file. Ask Munghard, or list it as a separate server-side mod instead of bundling it.

The alternative that needs no permission from anybody is to stop bundling: leave `CopyExtraPBOs`
off and install each as its own `@Mod` in the server's mod list, which is how their authors publish
them.

## Adding a file here

Drop the `.pbo`, its `.bisign` and the matching `.bikey` in this folder and add a row above. Keep
the table honest about the licence — an unknown licence is worth recording as unknown.
