# DayZ Battle Royale

Original work by Kegan: https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale

## Running a server

**Everything a server operator needs is in the [wiki](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki).**
That covers installation, mission files, every settings file with its defaults, and four example
setups. Start with [Quick start](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Quick-Start).

The mod is published on the Steam Workshop:
[stable](https://steamcommunity.com/sharedfiles/filedetails/?id=2871877303) and
[test](https://steamcommunity.com/sharedfiles/filedetails/?id=3173161748). The wiki describes this
repository's `main` branch, and a Workshop build can lag behind it.

| Topic | Wiki page |
|---|---|
| Installing, the `-mod=` order, signature keys, the restart loop | [Installing the server](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Installing-the-Server) |
| Missions, `db/globals.xml` (`CleanupLifetimeLimit`) | [Mission files](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Mission-Files) |
| `serverDZ.cfg` keys | [serverDZ.cfg](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/ServerDZ-cfg) |
| The settings files and mission overrides | [Configuration overview](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Configuration-Overview) |
| Zone sizes, round timers and tuning | [zone_settings.json](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Zone-Settings) |
| The optional native hooks (dzhooks) | [Native hooks](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Native-Hooks) |
| The next match without a server restart | [In-place reset](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/In-Place-Reset) |
| Log levels and common problems | [Logs & troubleshooting](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Logs-and-Troubleshooting) |
| Admin tools and permissions | [Admin guide](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Admin-Guide) |

## Dependencies

This mod requires the first three mods below. Their source code is available on GitHub — useful when
checking a dependency's API:

| Mod                      | Required | Source                                                    |
|--------------------------|----------|-----------------------------------------------------------|
| CF (Community Framework) | yes      | https://github.com/Arkensor/DayZ-CommunityFramework        |
| Dabs Framework           | yes      | https://github.com/InclementDab/DayZ-Dabs-Framework        |
| Community Online Tools   | yes      | https://github.com/Jacob-Mango/DayZ-CommunityOnlineTools   |
| DayZ Expansion           | no       | https://github.com/salutesh/DayZ-Expansion-Scripts         |

Without DayZ Expansion the mod gives up only the Expansion-specific touches: hiding Expansion's news
feed in the Esc menu, its version in the version line, airdrops (`ExpansionMissionModule`), and
suppressing Expansion's own kill feed. **When Expansion is loaded, list `@Vigrid-BattleRoyale` after
it in `-mod=`**: the load order is what lets this mod's classes override Expansion's.

## LICENSE

This work is licensed under the [DAYZ STANDALONE PUBLIC LICENSE SHARE ALIKE (DSPL-SA)](LICENSE).  
Additionally:

- You may not use this mod or any mod derived from it on a commercial server, such as a server with paid priority queue or paid access.
- You may not share or package this mod or any mod derived from it **in an obfuscated or encrypted manner**.

## Development

You can help develop this mod by creating issues and pull requests.  
Join our Discord server for discussions: https://discord.gg/egJWhJAf8b

- **Building:** [Building from source](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Building-from-Source)
  in the wiki is the short version. [`CLAUDE.md`](CLAUDE.md) has the full build, test and
  architecture notes, and [`Docs/`](Docs) has one file per subsystem.
- **Checks:** `python Tools/check.py -W` runs the static check suite, exactly as CI does on every pull
  request ([`Tools/README.md`](Tools/README.md)).
- **Logging:** with the Diag executable, trace logging is on by default. The `-br-*` launch parameters
  set the level ([Logs & troubleshooting](https://github.com/LeMyst/Vigrid-BattleRoyale/wiki/Logs-and-Troubleshooting#turning-up-the-log-level)).

## Matchmaking and Vigrid Network

The Vigrid Network provides matchmaking services for this mod. It's an **optional feature** that allows players to join matches directly from the main menu without searching the server browser.

### How it works:

- **Your own server:** When players join your non-Vigrid server, the client automatically falls back to default behavior.
- **Vigrid servers:** When players join a Vigrid server (either through matchmaking or the server browser), the client connects to the Vigrid API for match information.

Currently, the matchmaking service is limited to the Vigrid Network.
