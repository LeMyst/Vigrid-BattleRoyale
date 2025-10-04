#ifdef SERVER
class BattleRoyaleSpawnSelection: BattleRoyaleState
{
	int i_SpawnSelectionDuration = 30; // Duration in seconds
	int i_ExtraScreenTime = 2; // Extra time before the screen closes and switches to the next state
	bool b_ShowHeatMap = true; // Show spawn heatmap
	ref Timer m_SpawnSelectionTimer;

	private BattleRoyaleConfig m_Config;
    private BattleRoyaleGameData m_GameSettings;

	private ref set<int> spawn_colors;
	private ref map<string, int> player_spawn_colors = new map<string, int>; // Player ID -> Spawn color

	ref map<string, vector> spawnpoints;

    void BattleRoyaleSpawnSelection()
    {
    	m_Config = BattleRoyaleConfig.GetConfig();

        m_GameSettings = m_Config.GetGameData();

		i_SpawnSelectionDuration = m_GameSettings.spawn_selection_duration;
		i_ExtraScreenTime = m_GameSettings.spawn_selection_extra_time;
		b_ShowHeatMap = m_GameSettings.show_spawn_heatmap;

		spawn_colors = new set<int>;
//        spawn_colors.Insert(ARGB(255, 255, 179, 186));  // Light Pastel Pink
//        spawn_colors.Insert(ARGB(255, 255, 223, 186));  // Light Pastel Orange
//        spawn_colors.Insert(ARGB(255, 255, 255, 186));  // Light Pastel Yellow
//        spawn_colors.Insert(ARGB(255, 186, 255, 201));  // Light Pastel Green
//        spawn_colors.Insert(ARGB(255, 186, 225, 255));  // Light Pastel Blue
//        spawn_colors.Insert(ARGB(255, 213, 186, 255));  // Light Pastel Lavender
//        spawn_colors.Insert(ARGB(255, 255, 207, 225));  // Pastel Candy Pink
//        spawn_colors.Insert(ARGB(255, 178, 240, 230));  // Light Pastel Turquoise
//        spawn_colors.Insert(ARGB(255, 244, 179, 255));  // Light Pastel Violet
//        spawn_colors.Insert(ARGB(255, 229, 255, 179));  // Light Pastel Lime Green
		spawn_colors.Insert(ARGB(255, 255, 0, 0));  // Red
		spawn_colors.Insert(ARGB(255, 0, 255, 0));  // Green
//		spawn_colors.Insert(ARGB(255, 0, 0, 255));  // Blue
		spawn_colors.Insert(ARGB(255, 255, 255, 0));  // Yellow
		spawn_colors.Insert(ARGB(255, 255, 0, 255));  // Magenta
//		spawn_colors.Insert(ARGB(255, 0, 255, 255));  // Cyan
		spawn_colors.Insert(ARGB(255, 255, 127, 0));  // Orange
//		spawn_colors.Insert(ARGB(255, 127, 0, 255));  // Purple
//		spawn_colors.Insert(ARGB(255, 127, 255, 0));  // Lime
//		spawn_colors.Insert(ARGB(255, 0, 127, 255));  // Light Blue

		spawnpoints = new map<string, vector>();
    }

    override void Activate()
    {
        super.Activate();

        // Send RPC to all players to show spawn selection UI
        ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(GetDynamicStartingZone(m_Players.Count())).GetArea();
        vector v_FirstZoneCenter = spawn_area.GetCenter();
        float f_FirstZoneRadius = spawn_area.GetRadius();
        float f_SpawnSelectionRadius = m_GameSettings.spawn_selection_radius;

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnSelection", new Param4<int, float, vector, float>(i_SpawnSelectionDuration, f_SpawnSelectionRadius, v_FirstZoneCenter, f_FirstZoneRadius), true);

        // Add timer to deactivate this state after a certain time
        m_SpawnSelectionTimer = AddTimer(i_SpawnSelectionDuration + i_ExtraScreenTime, this, "OnSpawnSelectionTimeout", NULL, false);

        // Re-enable player input on clients
		EnableInput();

		// Disable player input on clients after 0.5 seconds (500ms) to reset the current animations (e.g. keep walking if they were walking)
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "DisableInput", 500, false);
//		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "DisablePlayerInput", NULL, true);

        // Listen to player spawn selection
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", this);
    }

    override void Deactivate()
    {
        // Stop the spawn selection timer if it's running
        if (m_SpawnSelectionTimer && m_SpawnSelectionTimer.IsRunning())
		{
			m_SpawnSelectionTimer.Stop();
		}

		// Remove the RPC listener
		GetRPCManager().RemoveRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected" );

		// Send RPC to all players to hide spawn selection UI
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "HideSpawnSelection", NULL, true);

		// Check for every party for player who didn't select a spawn point and assign them a random one from their party members
		// If no party members selected a spawn point, assign them a random spawn point from the spawnpoints map
		// If a solo player didn't select a spawn point, assign them a random spawn point from the spawnpoints map
		// If no spawn points are available, the next state will randomly teleport them
		BattleRoyaleUtils.Trace("Check for players who didn't select a spawn point and assign them a random one from their party members");
		ref array<PlayerBase> players = GetPlayers();
		foreach (PlayerBase player : players)
		{
			if( player.GetSpawnPos() == vector.Zero ) // Player didn't select a spawn point
			{
				BattleRoyaleUtils.Trace("Player " + player.GetIdentity().GetName() + " didn't select a spawn point, checking party members");
#ifdef Carim
				set<PlayerBase> groupMembers = GetGroup(player);
				if(groupMembers)
				{
					foreach (PlayerBase member : groupMembers)
					{
						if( member != player && member.GetSpawnPos() != vector.Zero ) // Found a party member who selected a spawn point
						{
							BattleRoyaleUtils.Trace("Assigning spawn point of " + member.GetIdentity().GetName() + " to " + player.GetIdentity().GetName());
							player.SetSpawnPos(member.GetSpawnPos());
							break;
						}
					}
				}

				// If still no spawn point assigned, use a random one from the map
				if( player.GetSpawnPos() == vector.Zero )
				{
					AssignRandomSpawnPoint(player, groupMembers);
				}
#else
				// No party mod - just assign a random spawn point from the map
				BattleRoyaleUtils.Trace("No party system available, assigning random spawn point");
				AssignRandomSpawnPoint(player, null);
#endif
			}
		}

        super.Deactivate();
    }

    override bool IsComplete()
    {
        return super.IsComplete();
    }

    override string GetName()
    {
        return "Spawn Selection State";
    }

    void EnableInput()
	{
		// Enable user input on all clients
		// Note: 'SetInput' expects 'true' to disable input and 'false' to enable input. So we pass 'false' here to enable input.
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(false), true);
	}

    void DisableInput()
    {
		// Disable user input on all clients
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true);
	}

    void OnSpawnSelectionTimeout()
	{
		// Deactivate this state and move to the next state
		Deactivate();
	}

	void OnPlayerSpawnSelected(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		Param1<vector> data;
		if( !ctx.Read( data ) )
		{
			Error("FAILED TO READ PLAYER SPAWN SELECTION RPC");
			return;
		}
		if ( type == CallType.Server )
		{
			BattleRoyaleUtils.Trace("Player selected spawn point: " + data.param1.ToString());
			PlayerBase pbTarget = PlayerBase.Cast(target);
			if(pbTarget)
			{
				BattleRoyaleUtils.Trace("Player " + pbTarget.GetIdentity().GetName() + " selected spawn point: " + data.param1.ToString());

				if (b_ShowHeatMap)
				{
					// Add the spawn point to the player's spawnpoints map
					spawnpoints.Set(pbTarget.GetIdentity().GetId(), data.param1);

					// Update the heatmap for the players
					array<vector> heatmap_points = new array<vector>;
					foreach (string id, vector point : spawnpoints)
					{
						heatmap_points.Insert(point);
					}
					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateHeatMap", new Param1<array<vector>>(heatmap_points), true );
				}

				pbTarget.SetSpawnPos(data.param1); // Set the spawn position for the player

#ifdef Carim
				int own_color = GetSpawnColor(pbTarget.GetIdentity().GetId());
#else
				int own_color = spawn_colors.Get(Math.RandomInt(0, spawn_colors.Count()));
#endif

				// Set the spawn position and color for the player
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnPoint", new Param3<PlayerBase, vector, int>(pbTarget, data.param1, own_color), true, pbTarget.GetIdentity(), pbTarget);

#ifdef Carim
				BattleRoyaleUtils.Trace("Test if player is in a group");
				if(GetGroup(pbTarget))
				{
					BattleRoyaleUtils.Trace("Player " + pbTarget.GetIdentity().GetName() + " is in a group, sharing spawn point with group");

					set<PlayerBase> groupMembers = GetGroup(pbTarget);
					foreach (PlayerBase member : groupMembers)
					{
						BattleRoyaleUtils.Trace("Sharing spawn point with " + member.GetIdentity().GetName());
						if (member != pbTarget)
						{
							GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnPoint", new Param3<PlayerBase, vector, int>(pbTarget, data.param1, GetSpawnColor(pbTarget.GetIdentity().GetId())), true, member.GetIdentity(), member);
						}
					}
				}
				else
				{
					BattleRoyaleUtils.Trace("Player " + pbTarget.GetIdentity().GetName() + " is not in a group, not sharing spawn point");
				}
#endif
			}
			else
			{
				BattleRoyaleUtils.Trace("Player is NULL in OnPlayerSpawnSelected");
			}
		}
	}

#ifdef Carim
	int GetSpawnColor(string playerId)
	{
		if(player_spawn_colors.Contains(playerId))
		{
			return player_spawn_colors.Get(playerId);  // Return the color if already assigned
		}

		int color = -1;  // Declare color variable

		if(MissionServer.Cast(GetGame().GetMission()).carimModelPartyParties.mutuals.Get(playerId) == NULL)
		{
			BattleRoyaleUtils.Trace("Player " + playerId + " is not in a group, assigning random color");
			color = spawn_colors.Get(Math.RandomInt(0, spawn_colors.Count()));  // Get a random color
			player_spawn_colors.Set(playerId, color);
			return color;
		}

		// Get the others teammates
		array<string> teammates = MissionServer.Cast(GetGame().GetMission()).carimModelPartyParties.mutuals.Get(playerId).ToArray();

		// Find a color that is not already used by the teammates
		int try_count = 0;
		while(true)
		{
			color = spawn_colors.Get(Math.RandomInt(0, spawn_colors.Count()));  // Get a random color

			if(try_count > 10)
			{
				BattleRoyaleUtils.Trace("Failed to find a unique spawn color after 10 tries");
				break;  // Break the loop if we can't find a unique color
			}
			try_count++;

			// Check if the color is already used by a teammate
			foreach(string teammateId : teammates)
			{
				if(player_spawn_colors.Contains(teammateId))
				{
					if(player_spawn_colors.Get(teammateId) == color)
					{
						color = -1;  // Color is already used by a teammate, try again
						break;
					}
				}
			}
			if(color != -1)
			{
				break;  // Color is not used by any teammate, break the loop
			}
		}

		// Store the color for the player
		player_spawn_colors.Set(playerId, color);

		return color;
	}
#endif
	/**
	 * Assigns a random spawn point to a player and optionally to their group members
	 * @param player The player to assign a spawn point to
	 * @param groupMembers Optional group members to also assign the spawn point to
	 */
	private void AssignRandomSpawnPoint(PlayerBase player, set<PlayerBase> groupMembers)
	{
		BattleRoyaleUtils.Trace("Assigning random spawn point");
		// Get a random spawn point from the spawnpoints map
		array<vector> heatmap_points = spawnpoints.GetValueArray();
		if( heatmap_points.Count() > 0 )
		{
			// Get a random spawn point
			vector random_spawn = heatmap_points.GetRandomElement();
			// Assign it to the player
			BattleRoyaleUtils.Trace("Assigning random spawn point to " + player.GetIdentity().GetName());
			player.SetSpawnPos(random_spawn);

			// If in a group, assign to all group members who haven't selected a spawn
			if(groupMembers)
			{
				foreach (PlayerBase member : groupMembers)
				{
					if( member != player && member.GetSpawnPos() == vector.Zero ) // Found a party member who didn't select a spawn point
					{
						BattleRoyaleUtils.Trace("Assigning random spawn point to " + member.GetIdentity().GetName());
						member.SetSpawnPos(random_spawn);
					}
				}
			}
		}
	}
}
