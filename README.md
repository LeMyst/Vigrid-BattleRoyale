# DayZ Battle Royale

Original work by Kegan: https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale

<!-- ToC generator: https://luciopaiva.com/markdown-toc/ -->

- [LICENSE](#license)
- [Development](#development)
- [Configuration files](#configuration-files)
    - [Mission files](#mission-files)
    - [Profile files](#profile-files)
- [Matchmaking and Vigrid Network](#matchmaking-and-vigrid-network)

# LICENSE

This work is licensed under the [DAYZ STANDALONE PUBLIC LICENSE SHARE ALIKE (DSPL-SA)](LICENSE).  
Additionally, you may not use this mod or any mod derived from this mod on a commercial server, such as a server with paid priority queue or paid access.  
And you may not share or package this mod or any mod derived from this mod in an obfuscated or encrypted manner.

# Development

You can help develop this mod by creating issues and pull requests.
There is a minimalistic Discord server for this mod: https://discord.gg/egJWhJAf8b

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

# Matchmaking and Vigrid Network

The Vigrid Network is a matchmaking service for this mod.  
It's a **non-mandatory feature** that allows players to join a match from the main menu, without having to search for a server in the server browser.  
If you are hosting your own server, when a player join your server, the client detects that it's not a Vigrid server and fallback to the default behavior.  
If the player join a Vigrid server from the server browser, the client will connect to the Vigrid API to get the match information.
At this date, the matchmaking service is limited to the Vigrid Network.
