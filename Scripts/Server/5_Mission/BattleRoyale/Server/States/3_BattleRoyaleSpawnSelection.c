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

	//--- Zone number Activate() advertised to the clients. Cached so the server validates incoming
	//--- selections against exactly the circle it told players about - GetDynamicStartingZone() is
	//--- called with m_Players.Count() here but with i_NumStartingPlayers elsewhere, and the two
	//--- can disagree.
	private int i_SpawnZoneNumber;

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
        i_SpawnZoneNumber = GetDynamicStartingZone(m_Players.Count());
        ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(i_SpawnZoneNumber).GetArea();
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

        // Listen to player spawn selection
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", this);
    }

    override void Deactivate()
    {
		//--- Deliberately FIRST, not last, unlike every sibling state.
		//---
		//--- Everything below can throw: a null list entry, a null identity, an RPC call,
		//--- AssignRandomSpawnPoint. EnfusionScript has no try/catch, so a throw simply abandons the
		//--- rest of the method. While this ran last, any such throw left b_IsActive true - and
		//--- IsComplete() returns !IsActive(), so the state could never report itself done and the
		//--- match hung in spawn selection until someone restarted the server process. Running it
		//--- first makes that failure mode structurally impossible rather than merely guarded
		//--- against: the worst a throw can now cost is some players' spawn assignments, and the
		//--- next state teleports those at random anyway.
		//---
		//--- Safe to hoist, on three counts:
		//---   - the base does nothing but StopTimers() and b_IsActive = false
		//---     (0_BattleRoyaleState.c:56-64). It never touches m_Players - its own comment says it
		//---     runs "BEFORE players are removed" - so GetPlayers() below is unaffected;
		//---   - nothing in this method reads b_IsActive, and Deactivate() runs to completion inside
		//---     a single call, so BattleRoyaleServer.Update() cannot see the flag mid-method;
		//---   - it does not cause a double Deactivate. The driver guards its own call with
		//---     `if(GetCurrentState().IsActive())` (BattleRoyaleServer.c:206-207), so once this has
		//---     run the driver skips straight to migrating players into the next state.
		super.Deactivate();

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
		//--- Every dereference below is guarded. A null list entry is possible, and PlayerIdentity
		//--- can be null while the PlayerBase is still alive - the workaround comment at
		//--- BattleRoyaleServer.c:238-239 documents exactly that case.
		//---
		//--- Hoisting super.Deactivate() to the top of this method already means a throw here can no
		//--- longer strand the match, so these guards are no longer load-bearing for that. They still
		//--- matter: a throw would abandon the loop, and every player after the faulty entry would
		//--- lose the teammate-spawn inheritance this whole state exists to provide.
		ref array<PlayerBase> players = GetPlayers();
		foreach (PlayerBase player : players)
		{
			if( !player )
				continue;
			if( player.GetSpawnPos() != vector.Zero ) // Player already selected a spawn point
				continue;

			//--- Resolved into a local instead of being inlined into the Trace calls: arguments are
			//--- evaluated whatever the log level, so an inlined GetIdentity().GetName() threw even
			//--- with logging switched off.
			string player_name = "<no identity>";
			if( player.GetIdentity() )
				player_name = player.GetIdentity().GetName();

			BattleRoyaleUtils.Trace("Player " + player_name + " didn't select a spawn point, checking party members");

			//--- Empty when the player has no teammates, so the loop below simply does not run
			//--- and the random fallback takes over. One code path either way.
			array<PlayerBase> groupMembers = new array<PlayerBase>();
#ifdef VIGRID_PARTY
			groupMembers = VigridPartyAPI.GetTeammates(player, players);
#endif

			foreach (PlayerBase member : groupMembers)
			{
				if( !member )
					continue;
				if( member.GetSpawnPos() == vector.Zero )
					continue;

				// Found a party member who selected a spawn point
				BattleRoyaleUtils.Trace("Assigning spawn point of a party member to " + player_name);
				player.SetSpawnPos(member.GetSpawnPos());
				break;
			}

			// If still no spawn point assigned, use a random one from the map
			if( player.GetSpawnPos() == vector.Zero )
			{
				AssignRandomSpawnPoint(player, groupMembers);
			}
		}
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

	//--- Server-side validation of a client-submitted spawn point.
	//--- Mirrors the checks in SpawnSelectionMenu.SelectSpawnPoint() - those run on the client only,
	//--- so a modified client can otherwise submit any coordinate on the map.
	//--- Deliberately does NOT call IsSafeForTeleport(): that runs a sphere cast plus a geometry box
	//--- test and is far too heavy for a per-click RPC. 4_BattleRoyalePrepare already runs
	//--- GetRandomSafePosition() around the chosen point to handle actual placement safety.
	private bool IsValidSpawnSelection(vector position)
	{
		int world_size = GetGame().GetWorld().GetWorldSize();

		if(position[0] < 0 || position[2] < 0 || position[0] > world_size || position[2] > world_size)
			return false;

		if(GetGame().SurfaceIsSea(position[0], position[2]))
			return false;

		if(GetGame().SurfaceIsPond(position[0], position[2]))
			return false;

		BattleRoyaleZone spawn_zone = BattleRoyaleZone.GetZone(i_SpawnZoneNumber);
		if(!spawn_zone)
			return true;

		BattleRoyalePlayArea spawn_area = spawn_zone.GetArea();
		if(!spawn_area)
			return true;

		// The client only applies its zone test when it received a usable circle. Match that, so a
		// degenerate zone makes the server permissive rather than rejecting every selection.
		float radius = spawn_area.GetRadius();
		if(radius <= 0)
			return true;

		// 2D distance, and the same 25 m tolerance the client applies, so that no selection the
		// client considered legitimate is rejected here.
		vector center = spawn_area.GetCenter();
		float dx = position[0] - center[0];
		float dz = position[2] - center[2];
		if(Math.Sqrt((dx * dx) + (dz * dz)) > (radius + 25))
			return false;

		return true;
	}

	//--- NOTE: `target` is deliberately ignored - it is client-chosen and could name any other
	//--- player. The subject is resolved from the engine-supplied `sender` instead.
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
			PlayerBase pbTarget = GetPlayerFromIdentity(sender);
			if(pbTarget)
			{
				BattleRoyaleUtils.Trace("Player " + sender.GetName() + " selected spawn point: " + data.param1.ToString());

				// Reject out-of-bounds / sea / out-of-zone selections. Returning without calling
				// SetSpawnPos leaves spawn_pos at the vector.Zero sentinel, so Deactivate() assigns
				// a fallback spawn exactly as it does for a player who never picked one.
				if(!IsValidSpawnSelection(data.param1))
				{
					BattleRoyaleUtils.Warn("Rejected invalid spawn selection " + data.param1.ToString() + " from " + sender.GetName() + " (" + sender.GetPlainId() + ")");
					return;
				}

				if (b_ShowHeatMap)
				{
					// Add the spawn point to the player's spawnpoints map
					spawnpoints.Set(sender.GetId(), data.param1);

					// Update the heatmap for the players
					array<vector> heatmap_points = new array<vector>;
					foreach (string id, vector point : spawnpoints)
					{
						heatmap_points.Insert(point);
					}
					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateHeatMap", new Param1<array<vector>>(heatmap_points), true );
				}

				pbTarget.SetSpawnPos(data.param1); // Set the spawn position for the player

				int own_color = GetSpawnColor(pbTarget);

				// Set the spawn position and color for the player
				GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnPoint", new Param3<PlayerBase, vector, int>(pbTarget, data.param1, own_color), true, sender, pbTarget);

#ifdef VIGRID_PARTY
				//--- Resolve the teammates once and reuse own_color. The previous version called
				//--- GetGroup() twice - each call rebuilt every group in the match - and recomputed
				//--- the colour for every member it sent to.
				array<PlayerBase> spawnMates = VigridPartyAPI.GetTeammates(pbTarget, GetPlayers());
				foreach (PlayerBase member : spawnMates)
				{
					if (!member.GetIdentity())
						continue;

					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnPoint", new Param3<PlayerBase, vector, int>(pbTarget, data.param1, own_color), true, member.GetIdentity(), member);
				}
#endif
			}
			else
			{
				BattleRoyaleUtils.Warn("Rejected spawn selection from an identity not tracked by the spawn selection state: " + BattleRoyaleServer.GetIdentityLogName(sender));
			}
		}
	}

	/**
	 * Spawn marker colour for a player, memoised so it stays stable for the whole selection.
	 *
	 * Party members are given a deterministic colour from their stable slot in the party, so two
	 * teammates can never collide and no retry loop is needed. The previous implementation looked
	 * the player up in the party mod's map keyed by leader, which meant every non-leader silently
	 * fell through to the random branch; and its retry loop tested the attempt counter after
	 * picking, so the eleventh failure could store -1 as the colour.
	 */
	int GetSpawnColor(PlayerBase player)
	{
		if(!player)
			return spawn_colors.Get(Math.RandomInt(0, spawn_colors.Count()));
		if(!player.GetIdentity())
			return spawn_colors.Get(Math.RandomInt(0, spawn_colors.Count()));

		string playerId = player.GetIdentity().GetPlainId();
		if(player_spawn_colors.Contains(playerId))
		{
			return player_spawn_colors.Get(playerId);  // Return the color if already assigned
		}

		int color = spawn_colors.Get(Math.RandomInt(0, spawn_colors.Count()));

#ifdef VIGRID_PARTY
		int slot = VigridPartyAPI.GetMemberIndex(player);
		if(slot >= 0)
		{
			// Collision-free while max_party_size stays within the palette size; several extra
			// colours sit commented out above if the party size is ever raised past it.
			color = spawn_colors.Get(slot % spawn_colors.Count());
		}
#endif

		player_spawn_colors.Set(playerId, color);

		return color;
	}

	/**
	 * Assigns a random spawn point to a player and optionally to their group members
	 * @param player The player to assign a spawn point to
	 * @param groupMembers Optional group members to also assign the spawn point to
	 */
	private void AssignRandomSpawnPoint(PlayerBase player, array<PlayerBase> groupMembers)
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
