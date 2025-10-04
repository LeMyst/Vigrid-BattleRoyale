#ifdef SERVER
// Spectator Info class to track data about dead players for spectator system
class SpectatorInfo
{
    string playerId;            // Steam ID of the dead player
    string playerName;          // Name of the player
    Object killedBy;            // Object that killed the player
    ref array<string> teammates; // List of teammate Steam IDs

    void SpectatorInfo(string id, string name, Object killer = null)
    {
        playerId = id;
        playerName = name;
        killedBy = killer;
        teammates = new array<string>();
    }
}

// Helper class for spectator system
class SpectatorSystem
{
    // Find a suitable spectate target prioritizing teammates
    static PlayerBase FindSpectateTarget(BattleRoyaleServer server, string playerSteamId)
    {
        if (!server || !playerSteamId || playerSteamId == "")
            return null;

        // Get spectator info for this player
        SpectatorInfo spectatorInfo = server.GetSpectatorInfo(playerSteamId);
        if (!spectatorInfo)
        {
            BattleRoyaleUtils.Trace("No spectator info found for player ID: " + playerSteamId);
            return null;
        }

        // Try to find a living teammate first
        PlayerBase target = FindTeammateTarget(server, spectatorInfo);
        if (target)
        {
            BattleRoyaleUtils.Trace("Found teammate target for spectating: " + target.GetIdentity().GetName());
            return target;
        }

        // If no teammates are alive, try to find the killer
        target = FindKillerTarget(server, spectatorInfo);
        if (target)
        {
            BattleRoyaleUtils.Trace("Found killer target for spectating: " + target.GetIdentity().GetName());
            return target;
        }

        // If no specific target found, choose a random player
        return FindRandomTarget(server, playerSteamId);
    }

    // Find a living teammate to spectate
    private static PlayerBase FindTeammateTarget(BattleRoyaleServer server, SpectatorInfo spectatorInfo)
    {
        if (!server || !spectatorInfo || !spectatorInfo.teammates || spectatorInfo.teammates.Count() == 0)
            return null;

        // Check if Carim mod is available for teammate tracking
#ifdef Carim
        MissionServer missionServer = MissionServer.Cast(GetGame().GetMission());
        if (missionServer && missionServer.carimModelPartyParties)
        {
            // Get current active players
            array<PlayerBase> activePlayers = server.GetCurrentState().GetPlayers();

            // Try to find a living teammate
            foreach (string teammateId : spectatorInfo.teammates)
            {
                foreach (PlayerBase player : activePlayers)
                {
                    if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == teammateId)
                    {
                        return player;
                    }
                }
            }
        }
#else
        BattleRoyaleUtils.Trace("Carim mod not available, skipping teammate search for spectator");
#endif

        return null;
    }

    // Find the player who killed this player
    private static PlayerBase FindKillerTarget(BattleRoyaleServer server, SpectatorInfo spectatorInfo)
    {
        if (!server || !spectatorInfo || !spectatorInfo.killedBy)
            return null;

        // Check if killer is a player and still alive
        PlayerBase killer = PlayerBase.Cast(spectatorInfo.killedBy);
        if (killer && killer.IsAlive())
        {
            // Verify killer is in current players list
            array<PlayerBase> activePlayers = server.GetCurrentState().GetPlayers();
            if (activePlayers.Find(killer) != -1)
            {
                return killer;
            }
        }

        return null;
    }

    // Find a random player to spectate when no other options are available
    private static PlayerBase FindRandomTarget(BattleRoyaleServer server, string excludePlayerId)
    {
        if (!server)
            return null;

        array<PlayerBase> activePlayers = server.GetCurrentState().GetPlayers();
        array<PlayerBase> potentialTargets = new array<PlayerBase>();

        // Collect all possible spectate targets (excluding self)
        foreach (PlayerBase player : activePlayers)
        {
            if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() != excludePlayerId)
            {
                potentialTargets.Insert(player);
            }
        }

        // Return a random player
        if (potentialTargets.Count() > 0)
        {
            return potentialTargets.GetRandomElement();
        }

        return null;
    }

    // Get teammates for a player using Carim party system
    static array<string> GetPlayerTeammates(string playerId)
    {
        array<string> teammates = new array<string>();

#ifdef Carim
        // Check if Carim mod is available
        MissionServer missionServer = MissionServer.Cast(GetGame().GetMission());
        if (missionServer && missionServer.carimModelPartyParties && missionServer.carimModelPartyParties.mutuals)
        {
            // Get teammates from Carim party system - using CarimSet instead of set<string>
            ref CarimSet teamSet = missionServer.carimModelPartyParties.mutuals.Get(playerId);
            if (teamSet)
            {
                return teamSet.ToArray();
            }
        }
#else
        BattleRoyaleUtils.Trace("Carim mod not available, no teammates will be found");
#endif

        return teammates;
    }
}
#endif
