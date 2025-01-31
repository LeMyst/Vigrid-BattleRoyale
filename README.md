# DayZ Battle Royale

Original work by Kegan: https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale

<!-- ToC generator: https://luciopaiva.com/markdown-toc/ -->

- [Configuration files](#configuration-files)
    - [Mission files](#mission-files)
    - [Profile files](#profile-files)

# LICENSE

This work is licensed under the [DAYZ STANDALONE PUBLIC LICENSE SHARE ALIKE (DSPL-SA)](LICENSE).  
Additionally, you may not use this mod or any mod derived from this mod on a commercial server, such as a server with paid priority queue or paid access.  
And you may not share or package this mod or any mod derived from this mod in an obfuscated or encrypted manner.

# Configuration files

Configuration files are separated in two categories: `mission` and `profile`.

## Mission files

Mission are specific to a mission/map and contains settings related to the mission itself, like POIs, spawns, zones, etc.  
They are stored in the mission folder, eg: `mpmissions/DayZBattleRoyale.ChernarusPlus/Vigrid-BattleRoyale`.

`pois_settings.json`: Overriding spawn points.  
`spawns_settings.json`: Lobby spawn point, players spawn points, avoiding cities spawn points.  
`zone_settings.json`: Number of zones, zone size and duration, end zone configuration.

## Profile files

Profile are more general settings like notifications settings.  
They are stored in the profile folder, eg: `profiles/Vigrid-BattleRoyale`.

`lobby_settings.json`: Lobby configuration.  
`general_settings.json`: Notification, zone damage, airdrop, etc.  
`server_settings.json`: Vigrid API configuration (non-mandatory).  
