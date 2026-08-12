# DayZ Battle Royale

Original work by Kegan: https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale

## Dependencies

This mod requires the following mods. Their source code is available on GitHub — useful when
checking a dependency's API:

| Mod                      | Source                                                    |
|--------------------------|-----------------------------------------------------------|
| CF (Community Framework) | https://github.com/Arkensor/DayZ-CommunityFramework        |
| Dabs Framework           | https://github.com/InclementDab/DayZ-Dabs-Framework        |
| Community Online Tools   | https://github.com/Jacob-Mango/DayZ-CommunityOnlineTools   |
| DayZ Expansion           | https://github.com/salutesh/DayZ-Expansion-Scripts         |

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
| `zone_settings.json`    | Number of zones, zone size and duration, end zone configuration, hot zones |
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

## Zone sizes and round timers

`static_sizes`, `static_timers` and `min_players` in `zone_settings.json` are **ordered smallest zone
first**: index 0 is the tight final circle, the last index is the widest opening one. `num_zones`
picks that many entries **from the small end**, so lowering it drops the *largest* circles and always
keeps the endgame.

### Sizing the opening circle to your map

Aim for `r_max ≈ 0.22 × world_size`. PUBG's Erangel is 8 km across with a first circle of about 2 km
radius, and past `0.25 × world_size` the opening circle has so little room inside the map that its
centre lands in the same place every match — the server warns when you cross that line.

| Map | World size | Suggested `r_max` |
|---|---|---|
| ChernarusPlus | 15360 | 3375 (the shipped default is already right) |
| Sakhal | 15360 | 3375 |
| Livonia | 12800 | 2800 |

Because `zone_settings.json` supports a mission override and the mission is per-map, per-map sizes
need nothing but a `zone_settings.json` in the mission folder. Setting `scale_sizes_to_world` to `1`
does it automatically instead: it holds the final circle fixed and scales only the span above it, so
the endgame stays the size you tuned while the opening circle follows the map.

**Validate your config before you ship it.** Set `zone_selftest_runs` to `200` and boot once — the
server generates 200 throwaway sets of circles, reports how many failed, how much backtracking and
tier escalation each needed, and how widely the final circle moves, then plays normally. That answers
"can this configuration ever get stuck on this map" in one boot.

### Two curves worth considering

Both are alternatives to the shipped values, not new defaults — copy one in if you want it. Timers
come from `travel / (0.8 × 4.5 m/s)` with a 120 s floor, where `travel` is the distance from the far
edge of the current circle to the new one; 4.5 m/s is sustained cross-country speed, not the 6.5 m/s
sprint. The shipped timers already sit within 2% of that formula for the two longest rounds.

**A — smoothed, `num_zones` 6, ~34.6 min.** Minimal change. The shipped curve has a cliff at
562 → 140 that deletes 94% of the play area in a single round; this moves the sharpest compression to
the last round, where the travel is only 110 m and it is cheap.

```json
"num_zones": 6,
"static_sizes":  [40, 150, 450, 1050, 2200, 3375, 4500],
"static_timers": [120, 150, 260, 495, 510, 420, 495],
"min_players":   [10, 10, 10, 11, 22, 33, 44]
```

**B — PUBG-styled, `num_zones` 8, ~32.4 min.** A steady ~0.6 shrink ratio through the mid-game, so
more rounds that are each shorter — more rotations, less waiting. Costs two extra round states.

```json
"num_zones": 8,
"static_sizes":  [60, 170, 320, 540, 880, 1400, 2200, 3375],
"static_timers": [90, 90, 90, 150, 225, 345, 505, 360],
"min_players":   [10, 10, 10, 10, 14, 22, 33, 44]
```

Note the total number of circles does **not** change how far the zone can travel — the per-step
budgets telescope, so total reach depends only on `r_max − r_min`. Extra circles change pacing, not
reachability.

## Matchmaking and Vigrid Network

The Vigrid Network provides matchmaking services for this mod. It's an **optional feature** that allows players to join matches directly from the main menu without searching the server browser.

### How it works:

- **Your own server:** When players join your non-Vigrid server, the client automatically falls back to default behavior.
- **Vigrid servers:** When players join a Vigrid server (either through matchmaking or the server browser), the client connects to the Vigrid API for match information.

Currently, the matchmaking service is limited to the Vigrid Network.