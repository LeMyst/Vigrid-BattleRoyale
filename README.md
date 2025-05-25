# DayZ Battle Royale

Original work by Kegan: https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale

## LICENSE

This work is licensed under the [DAYZ STANDALONE PUBLIC LICENSE SHARE ALIKE (DSPL-SA)](LICENSE).  
Additionally:

- You may not use this mod or any mod derived from it on a commercial server, such as a server with paid priority queue or paid access.
- You may not share or package this mod or any mod derived from it **in an obfuscated or encrypted manner**.

## Development

You can help develop this mod by creating issues and pull requests.  
Join our Discord server for discussions: https://discord.gg/egJWhJAf8b

### Debugging

The mod supports various logging levels that can be enabled via launch parameters. Use only one parameter at a time:

| Parameter   | Description                                                     |
|-------------|-----------------------------------------------------------------|
| `-br-warn`  | Displays warning messages only                                  |
| `-br-info`  | Displays information messages and warnings                      |
| `-br-debug` | Displays debug messages, information, and warnings              |
| `-br-trace` | Displays all messages (trace, debug, information, and warnings) |

**Note for Diag users:** When using the Diag executable, trace logging is enabled by default. You can use the parameters above to reduce verbosity if needed.

## Configuration Files

Configuration files are stored in the profile folder and can be overridden with files in the mission folder.

### Location and Structure

Files are saved in the `Vigrid-BattleRoyale` folder inside the profile directory:

| Filename                | Purpose                                                                    |
|-------------------------|----------------------------------------------------------------------------|
| `pois_settings.json`    | Overriding spawn points                                                    |
| `spawns_settings.json`  | Lobby spawn point, player spawn points, city avoidance settings            |
| `zone_settings.json`    | Number of zones, zone size and duration, end zone configuration            |
| `lobby_settings.json`   | Lobby configuration                                                        |
| `general_settings.json` | Notification, zone damage, airdrop settings, etc.                          |
| `server_settings.json`  | Vigrid API configuration (optional, cannot be overridden by mission files) |

### Mission-Specific Overrides

You can override default configurations with mission-specific settings by creating a folder named `Vigrid-BattleRoyale` in your mission folder.

Example path: `mpmissions/DayZBattleRoyale.ChernarusPlus/Vigrid-BattleRoyale/`

#### Example Override

To override the `avoid_city_spawn` setting from `spawns_settings.json`, create this file in your mission folder with the following content:

```json
{
  "avoid_city_spawn": [
    "Settlement_Dubovo",
    "Settlement_Vysotovo"
  ]
}
```

This will override only the `avoid_city_spawn` setting while preserving other settings from the profile folder's configuration.

## Matchmaking and Vigrid Network

The Vigrid Network provides matchmaking services for this mod. It's an **optional feature** that allows players to join matches directly from the main menu without searching the server browser.

### How it works:

- **Your own server:** When players join your non-Vigrid server, the client automatically falls back to default behavior.
- **Vigrid servers:** When players join a Vigrid server (either through matchmaking or the server browser), the client connects to the Vigrid API for match information.

Currently, the matchmaking service is limited to the Vigrid Network.