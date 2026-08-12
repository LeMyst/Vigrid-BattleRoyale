#ifdef SERVER
class BattleRoyaleSpawnSelection: BattleRoyaleState
{
	int i_SpawnSelectionDuration = 30; // Duration in seconds
	int i_ExtraScreenTime = 2; // Extra time before the screen closes and switches to the next state
	bool b_ShowHeatMap = true; // Show spawn heatmap
	ref Timer m_SpawnSelectionTimer;

	private BattleRoyaleConfig m_Config;
    private BattleRoyaleLobbyData m_LobbySettings;

	private ref set<int> spawn_colors;
	private ref map<string, int> player_spawn_colors = new map<string, int>; // Player ID -> Spawn color

	ref map<string, vector> spawnpoints;

	//--- Zone number Activate() advertised to the clients. Cached so the server validates incoming
	//--- selections against exactly the circle it told players about.
	private int i_SpawnZoneNumber;

    void BattleRoyaleSpawnSelection()
    {
    	m_Config = BattleRoyaleConfig.GetConfig();

        m_LobbySettings = m_Config.GetLobbyData();

		i_SpawnSelectionDuration = m_LobbySettings.spawn_selection_duration;
		i_ExtraScreenTime = m_LobbySettings.spawn_selection_extra_time;
		b_ShowHeatMap = m_LobbySettings.show_spawn_heatmap;

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
        //--- i_NumStartingPlayers, not the live count: the rounds decide which zone to skip to from
        //--- the countdown snapshot, so using the live count here can advertise a different circle.
        i_SpawnZoneNumber = GetDynamicStartingZone(i_NumStartingPlayers);
        ref BattleRoyalePlayArea spawn_area = BattleRoyaleZone.GetZone(i_SpawnZoneNumber).GetArea();
        vector v_FirstZoneCenter = spawn_area.GetCenter();
        float f_FirstZoneRadius = spawn_area.GetRadius();
        float f_SpawnSelectionRadius = m_LobbySettings.spawn_selection_radius;

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowSpawnSelection", new Param4<int, float, vector, float>(i_SpawnSelectionDuration, f_SpawnSelectionRadius, v_FirstZoneCenter, f_FirstZoneRadius), true);

        // Add timer to deactivate this state after a certain time
        m_SpawnSelectionTimer = AddTimer(i_SpawnSelectionDuration + i_ExtraScreenTime, this, "OnSpawnSelectionTimeout", NULL, false);

        // Re-enable player input on clients
		EnableInput();

		// Disable player input on clients after 0.5 seconds (500ms) to reset the current animations (e.g. keep walking if they were walking)
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "DisableInput", 500, false);

		//--- Gather parties on the same delay, and for a different reason: by now the map menu is up
		//--- on every client and covers the character entirely, so the teleport is not seen. Doing it
		//--- in Activate() instead produced a visible snap in the moment before the map opened.
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "GatherPartiesOnLeader", 500, false);

        // Listen to player spawn selection
        GetRPCManager().AddRPC( RPC_DAYZBRSERVER_NAMESPACE, "OnPlayerSpawnSelected", this);
    }

    override void Deactivate()
    {
		//--- Deliberately FIRST, not last, unlike every sibling state.
		//---
		//--- Everything below can throw: a null list entry, a null identity, an RPC call, the party
		//--- resolution. EnfusionScript has no try/catch, so a throw simply abandons the
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

		//--- Give every party ONE drop point and hand it to the members who did not pick one, so a
		//--- player who was AFK or undecided lands with their squad instead of somewhere across the
		//--- circle. Ordered by intent, and resolved per party rather than per player - the previous
		//--- version asked each undecided member for "the first teammate who picked", which had no
		//--- notion of a leader and could in principle answer differently for two members of the
		//--- same party.
		//---
		//--- When nobody in a party picked at all, this deliberately assigns NOTHING. Leaving the
		//--- whole group on the vector.Zero sentinel is what routes it into
		//--- 4_BattleRoyalePrepare.TeleportGroup(), which searches one village-or-safe position for
		//--- the party and scatters them within 5 m of it. Inventing a point here would only get in
		//--- the way of that, and would skip the village preference entirely.
		//---
		//--- Solo players are untouched: GetGroups() is a partition that returns them as groups of
		//--- one, the Count() < 2 test skips them, and Prepare gives them the ordinary random spawn.
		//---
		//--- Every dereference below is guarded. A null list entry is possible, and PlayerIdentity
		//--- can be null while the PlayerBase is still alive - the workaround comment at
		//--- BattleRoyaleServer.c:238-239 documents exactly that case.
		//---
		//--- Hoisting super.Deactivate() to the top of this method already means a throw here can no
		//--- longer strand the match, so these guards are no longer load-bearing for that. They still
		//--- matter: a throw would abandon the loop, and every party after the faulty entry would
		//--- lose the spawn inheritance this whole block exists to provide.
#ifdef VIGRID_PARTY
		BattleRoyaleUtils.Trace("Resolving a shared spawn point for every party with undecided members");

		ref array<PlayerBase> players = GetPlayers();
		array<ref array<PlayerBase>> groups = VigridPartyAPI.GetGroups(players);

		for(int g = 0; g < groups.Count(); g++)
		{
			array<PlayerBase> group = groups.Get(g);
			if( group.Count() < 2 )
				continue;

			vector anchor = ResolvePartyAnchor(group, players);
			if( anchor == vector.Zero )
			{
				BattleRoyaleUtils.Info("Spawn selection: no member of a party of " + group.Count() + " picked a spawn point - the whole party will be dropped together at random.");
				continue;
			}

			int inherited = 0;
			for(int m = 0; m < group.Count(); m++)
			{
				PlayerBase member = group.Get(m);
				if( !member )
					continue;
				if( member.GetSpawnPos() != vector.Zero ) // Member already picked their own
					continue;

				member.SetSpawnPos(anchor);
				inherited++;
			}

			if( inherited > 0 )
				BattleRoyaleUtils.Info("Spawn selection: " + inherited + " member(s) of a party of " + group.Count() + " inherited the spawn point " + anchor.ToString());
		}
#endif
    }

#ifdef VIGRID_PARTY
	/**
	 * The single drop point a party shares, or vector.Zero when nobody in it picked one.
	 *
	 * Only ever reads GetSpawnPos(), never writes - the caller decides who inherits, so that
	 * "nobody picked" stays distinguishable from "everybody already has a point".
	 */
	private vector ResolvePartyAnchor(array<PlayerBase> group, array<PlayerBase> population)
	{
		//--- The leader speaks for the party. GetLeader() answers null when the leader is offline or
		//--- otherwise absent from the roster - which is exactly when the party should fall back to
		//--- its own members rather than be given a stand-in. Passing group.Get(0) is the idiom
		//--- GatherPartiesOnLeader() uses; the returned leader may well be that same player.
		PlayerBase leader = VigridPartyAPI.GetLeader(group.Get(0), population);
		if( leader && leader.GetSpawnPos() != vector.Zero )
		{
			BattleRoyaleUtils.Trace("Party anchor is the leader's own pick: " + leader.GetSpawnPos().ToString());
			return leader.GetSpawnPos();
		}

		//--- No leader pick. Draw at random among the picks the party did make, so a party that
		//--- disagreed does not systematically favour whoever happens to sit first in the roster.
		array<vector> picks = new array<vector>();
		for(int i = 0; i < group.Count(); i++)
		{
			PlayerBase member = group.Get(i);
			if( !member )
				continue;
			if( member.GetSpawnPos() == vector.Zero )
				continue;

			picks.Insert(member.GetSpawnPos());
		}

		if( picks.Count() == 0 )
			return vector.Zero;

		vector anchor = picks.GetRandomElement();
		BattleRoyaleUtils.Trace("Party anchor drawn at random from " + picks.Count() + " member pick(s): " + anchor.ToString());

		return anchor;
	}
#endif

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

    /**
     *  Put every party member next to their leader for the duration of spawn selection.
     *
     *  Party-only voice alone is not enough to let a squad coordinate here. CGame.MutePlayer is
     *  *subtractive*: it removes hearing from inside the engine's proximity set and can never add
     *  it. Voice is also clamped to Whisper until the match starts. So a member who drifted to the
     *  far side of the lobby - or who connected late and was placed elsewhere - is inaudible to
     *  their own party no matter what the mute matrix permits. Standing them together is the only
     *  thing that actually makes party voice usable.
     *
     *  Safe to move players here: the lobby leash lives on BattleRoyaleDebugState.OnPlayerTick and
     *  this state is a plain BattleRoyaleState, so nothing drags them back. Input is already frozen,
     *  so nobody walks away either. SetPosition matches what the leash itself uses.
     *
     *  Solo players are untouched - GetGroups() returns them as groups of one.
     *
     *  Called 500 ms after Activate() rather than from it, so the map menu is already covering every
     *  client's screen and the teleport is invisible. Not protected, because the call queue reaches
     *  it by name. Being deferred, it can in principle fire after the state ended, hence the guard.
     */
    void GatherPartiesOnLeader()
    {
#ifdef VIGRID_PARTY
        if (!IsActive())
            return;

        if (!m_LobbySettings.gather_party_for_spawn_selection)
            return;

        array<PlayerBase> population = GetPlayers();
        array<ref array<PlayerBase>> groups = VigridPartyAPI.GetGroups(population);

        int gathered = 0;

        for (int g = 0; g < groups.Count(); g++)
        {
            array<PlayerBase> group = groups.Get(g);
            if (group.Count() < 2)
                continue;

            //--- Anchor on the leader when they are here. If they are not - offline, or dropped
            //--- between the countdown and now - the first member anchors instead, so the party
            //--- still ends up together rather than being left scattered.
            PlayerBase anchor = VigridPartyAPI.GetLeader(group.Get(0), population);
            if (!anchor)
                anchor = group.Get(0);
            if (!anchor)
                continue;

            vector anchor_pos = anchor.GetPosition();

            for (int m = 0; m < group.Count(); m++)
            {
                PlayerBase member = group.Get(m);
                if (!member)
                    continue;
                if (member == anchor)
                    continue;

                vector spot = "0 0 0";
                spot[0] = anchor_pos[0] + Math.RandomFloatInclusive(-BR_PARTY_GATHER_RADIUS, BR_PARTY_GATHER_RADIUS);
                spot[2] = anchor_pos[2] + Math.RandomFloatInclusive(-BR_PARTY_GATHER_RADIUS, BR_PARTY_GATHER_RADIUS);
                spot[1] = GetGame().SurfaceY(spot[0], spot[2]);

                member.SetPosition(spot);

                //--- Face the anchor, so a gathered party visibly forms a huddle. vector.Direction
                //--- returns an un-normalized difference, and SetDirection wants a unit vector, so
                //--- flatten then normalize. Skipped outright if the two land on the same spot,
                //--- because normalizing a zero vector is undefined.
                vector facing = vector.Direction(spot, anchor_pos);
                facing[1] = 0;
                if (facing.Length() > 0.001)
                    member.SetDirection(facing.Normalized());

                gathered++;
            }
        }

        if (gathered > 0)
            BattleRoyaleUtils.Info(string.Format("Spawn selection: gathered %1 player(s) onto their party leader", gathered));
#endif
    }

    void OnSpawnSelectionTimeout()
	{
		// Deactivate this state and move to the next state.
		//--- Deferred: this is a timer callback, i.e. inside TimerQueue.Tick. See
		//--- BattleRoyaleState.DeactivateDeferred().
		DeactivateDeferred();
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

	//--- AssignRandomSpawnPoint() used to sit here. It copied a random entry out of `spawnpoints`,
	//--- i.e. some *stranger's* chosen drop, onto anyone left undecided - which put an AFK player
	//--- right on top of whoever picked that spot, and did nothing at all when the map was empty
	//--- (nobody in the match picked) or never filled (b_ShowHeatMap off, see OnPlayerSpawnSelected).
	//--- Parties are now resolved from their own members in Deactivate(), and an undecided solo or
	//--- an undecided whole party is handed to 4_BattleRoyalePrepare, which gives them a proper
	//--- village spawn instead. `spawnpoints` is now purely the heatmap feed.
}
