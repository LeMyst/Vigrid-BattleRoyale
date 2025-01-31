# DayZ Battle Royale

Original work by Kegan: https://gitlab.desolationredux.com/DayZ/DayZBR-Mod/BattleRoyale

<!-- ToC generator: https://luciopaiva.com/markdown-toc/ -->

- [Configuration files](#configuration-files)
    - [Mission files](#mission-files)
    - [Profile files](#profile-files)
- [Matchmaking and Vigrid Network](#matchmaking-and-vigrid-network)

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
