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
    	BattleRoyaleUtils.Trace("Finding spectate target for player ID: " + playerSteamId);
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
            BattleRoyaleUtils.Trace("Found living teammate to spectate: " + target.GetIdentity().GetName());
            return target;
        }

        // If no living teammates, find the killer of the last teammate who died
        target = FindLastTeammateKillerTarget(server, spectatorInfo);
        if (target)
        {
            BattleRoyaleUtils.Trace("Found last teammate's killer to spectate: " + target.GetIdentity().GetName());
            return target;
        }

        // If no last teammate killer is found or not alive, try the player's direct killer
        target = FindDirectKillerTarget(server, spectatorInfo);
        if (target)
        {
            BattleRoyaleUtils.Trace("Found direct killer to spectate: " + target.GetIdentity().GetName());
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
					// Check if this player is a teammate and still alive
					if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == teammateId && player.IsAlive())
					{
						BattleRoyaleUtils.Trace("Found living teammate to spectate: " + player.GetIdentity().GetName());
						return player; // Found a living teammate
					}
				}
			}
		}
	#else
		BattleRoyaleUtils.Trace("Carim mod not available, skipping teammate search for spectator");
	#endif

		return null;
	}

    // Find the player who killed this player or follow the killer chain
    private static PlayerBase FindDirectKillerTarget(BattleRoyaleServer server, SpectatorInfo spectatorInfo)
    {
        if (!server || !spectatorInfo)
            return null;

        // Get current active players
        array<PlayerBase> activePlayers = server.GetCurrentState().GetPlayers();

        // First try direct killer
        if (spectatorInfo.killedBy)
        {
            // Check if killer is a player and still alive
            PlayerBase killer = PlayerBase.Cast(spectatorInfo.killedBy);
            if (killer && killer.IsAlive())
            {
                // Verify killer is in current players list
                if (activePlayers.Find(killer) != -1)
                {
                    BattleRoyaleUtils.Trace("Found direct killer to spectate: " + killer.GetIdentity().GetName());
                    return killer;
                }
            }
        }

        return null;
    }

    // Find the player who killed the last teammate or follow the killer chain
    private static PlayerBase FindLastTeammateKillerTarget(BattleRoyaleServer server, SpectatorInfo spectatorInfo)
    {
        if (!server || !spectatorInfo)
            return null;

        // Get current active players
        array<PlayerBase> activePlayers = server.GetCurrentState().GetPlayers();

        // If no teammates are alive, we cannot find the last teammate's killer
        if (spectatorInfo.teammates && spectatorInfo.teammates.Count() > 0)
        {
            // Set to track visited players to avoid infinite loops
            ref set<string> visitedPlayers = new set<string>();
            visitedPlayers.Insert(spectatorInfo.playerId); // Mark self as visited

            // Try to find the killer of any teammate who died
            return FindKillerChain(server, spectatorInfo, visitedPlayers);
        }

        return null;
    }

    // Recursively follow the killer chain through teammates and their killers
    private static PlayerBase FindKillerChain(BattleRoyaleServer server, SpectatorInfo spectatorInfo, set<string> visitedPlayers)
    {
        if (!server || !spectatorInfo)
            return null;

        array<PlayerBase> activePlayers = server.GetCurrentState().GetPlayers();
        PlayerBase chainTarget;

        // Check all teammates
        if (spectatorInfo.teammates)
        {
            foreach (string teammateId : spectatorInfo.teammates)
            {
                // Skip if we've already visited this player
                if (visitedPlayers.Find(teammateId) != -1)
                    continue;

                // Mark as visited to prevent cycles
                visitedPlayers.Insert(teammateId);

                // Check if teammate is alive
                foreach (PlayerBase player : activePlayers)
                {
                    if (player && player.GetIdentity() && player.GetIdentity().GetPlainId() == teammateId && player.IsAlive())
                    {
                        BattleRoyaleUtils.Trace("Found living teammate through killer chain: " + player.GetIdentity().GetName());
                        return player;
                    }
                }

                // If teammate is dead, check their spectator info
                SpectatorInfo teammateInfo = server.GetSpectatorInfo(teammateId);
                if (teammateInfo)
                {
                    // Check teammate's direct killer
                    if (teammateInfo.killedBy)
                    {
                        PlayerBase killer = PlayerBase.Cast(teammateInfo.killedBy);
                        if (killer && killer.IsAlive())
                        {
                            // Verify killer is in current players list
                            if (activePlayers.Find(killer) != -1)
                            {
                                BattleRoyaleUtils.Trace("Found teammate's killer to spectate: " + killer.GetIdentity().GetName());
                                return killer;
                            }
                        }

                        // If the killer is a player but not alive or not found in active players,
                        // we need to check the killer's killer (continue the chain)
                        if (killer && killer.GetIdentity())
                        {
                            string killerId = killer.GetIdentity().GetPlainId();
                            if (visitedPlayers.Find(killerId) == -1)
                            {
                                // Mark as visited
                                visitedPlayers.Insert(killerId);

                                // Get the killer's SpectatorInfo to follow the chain
                                SpectatorInfo killerInfo = server.GetSpectatorInfo(killerId);
                                if (killerInfo)
                                {
                                    chainTarget = FindKillerChain(server, killerInfo, visitedPlayers);
                                    if (chainTarget)
                                        return chainTarget;
                                }
                            }
                        }
                    }

                    // Recursively check this teammate's spectator info
                    chainTarget = FindKillerChain(server, teammateInfo, visitedPlayers);
                    if (chainTarget)
                        return chainTarget;
                }
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
