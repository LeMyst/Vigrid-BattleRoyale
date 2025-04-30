#ifdef SERVER
class BattleRoyaleState: Timeable
{
    protected ref array<PlayerBase> m_Players;
    protected bool b_IsActive;
    protected bool b_IsPaused;
    protected bool b_IsDebug;
    static int i_StartingZone = 1; // Default zone is the biggest one

#ifdef Carim
    bool hide_players_endgame = false;
#endif

    string GetName()
    {
        return "Unknown State";
    }

    void BattleRoyaleState()
    {
        m_Players = new array<PlayerBase>();

        b_IsActive = false;
        b_IsPaused = false;
        b_IsDebug = false;

        // Resend game info every 5 seconds
        AddTimer(5.0, this, "ResendGameInfo", NULL, true);

#ifdef Carim
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            hide_players_endgame = m_GameSettings.hide_players_endgame;
        }
#endif
    }

    void Update(float timeslice)
    {

    }

    //state controls
    void Activate()
    {
        //Note: this is called AFTER players are added
        b_IsActive = true;
    }

    void Deactivate()
    {
        //Note: this is called BEFORE players are removed
        //--- stop all repeating timers
        StopTimers();

        b_IsActive = false;
    }

    bool IsActive()
    {
        return b_IsActive;
    }

    bool IsPaused()
    {
        return b_IsPaused;
    }

    void Pause()
    {
        b_IsPaused = true;
    }

    void Resume()
    {
        b_IsPaused = false;
    }

    bool IsComplete()
    {
        //state cannot complete if paused
        if(b_IsPaused)
            return false;

        return !IsActive();
    }

    bool SkipState(BattleRoyaleState _previousState)  //if true, we will skip activating/deactivating this state entirely
    {
        return false;
    }

    ref array<PlayerBase> GetPlayers()
    {
        return m_Players;
    }

    void AddPlayer(PlayerBase player)
    {
        m_Players.Insert( player );
        OnPlayerCountChanged();
    }

    void RemovePlayer(PlayerBase player)
    {
        m_Players.RemoveItem(player);
        OnPlayerCountChanged();
    }

    ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> result_array = new array<PlayerBase>();
        result_array.InsertAll(m_Players);
        m_Players.Clear();
        OnPlayerCountChanged();
        return result_array;
    }

    bool ContainsPlayer(PlayerBase player)
    {
        return (m_Players.Find(player) >= 0);
    }

    void OnPlayerTick(PlayerBase player, float timeslice)
    {
#ifdef SPECTATOR
        if(player)
        {
            if(player.UpdateHealthStatsServer( player.GetHealth01("", "Health"), player.GetHealth01("", "Blood"), timeslice ))
            {
                //BattleRoyaleUtils.Trace("Player Health Changed! Syncing Network...");
                //the player's stats changed (sync it over the network)
                GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateEntityHealth", new Param2<float, float>( player.health_percent, player.blood_percent ), true, NULL, player);
            }

            //UpdateHealthStatsServer will ensure that time_since_last_net_sync == 0 when a potential health update gets triggered. We can use this same window to also send map entity data updates
            //TODO: look for a more performant way of doing this maybe?
            if(player.time_since_last_net_sync == 0)
            {
                GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateMapEntityData", new Param4<string, string, vector, vector>( player.GetIdentity().GetId(), player.GetIdentityName(), player.GetPosition(), player.GetDirection() ), true);
            }
        }
#endif
    }

	void ResendGameInfo()
    {
    	BattleRoyaleUtils.Trace("BattleRoyaleState::ResendGameInfo()");

        if(IsActive())
        {
        	// Send SetPlayerCount and SetTopPosition
            OnPlayerCountChanged();
        }
    }

    //player count changed event handler
    protected void OnPlayerCountChanged()
    {
        //BattleRoyaleUtils.Trace("OnPlayerCountChanged()");
        if(IsActive())
        {
            int nb_players, nb_groups;

			nb_players = GetPlayers().Count();
#ifdef Carim
			int groups_count = GetGroupsCount();

			if(nb_players < 10 && hide_players_endgame && !b_IsDebug)
				nb_groups = -1;
			else
				nb_groups = groups_count;
			UpdateTopPosition( groups_count );
#else
			nb_groups = -2;
			UpdateTopPosition( nb_players );
#endif

			//BattleRoyaleUtils.Trace(string.Format("OnPlayerCountChanged: %1 %2", nb_players, nb_groups));
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", new Param2<int, int>( nb_players, nb_groups ), true);
		}
	}

    void UpdateTopPosition( int position )
    {
		for(int i = 0; i < GetPlayers().Count(); i++)
		{
			PlayerBase player = GetPlayers()[i];
			player.SetBRPosition( position );
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetTopPosition", new Param1<int>( position ), true, player.GetIdentity() );
		}
    }

    //CreateNotification( ref StringLocaliser title, ref StringLocaliser text, string icon, int color, float time, PlayerIdentity identity ) ()
    void MessagePlayers(string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
    {
        StringLocaliser text = new StringLocaliser( message );
        ExpansionNotification(DAYZBR_MSG_TITLE, text, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, time).Create();
    }

    void MessagePlayer(PlayerBase player, string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
    {
        if(player)
        {
            PlayerIdentity identity = player.GetIdentity();
            if(identity)
            {
                StringLocaliser text = new StringLocaliser( message );
                ExpansionNotification(DAYZBR_MSG_TITLE, text, DAYZBR_MSG_IMAGE, COLOR_EXPANSION_NOTIFICATION_INFO, time).Create(identity);
            }
        }
    }

	/*
	 * Send a message to all players, with standard message duration
	 * @param message The message to send
	 */
    void MessagePlayersUntranslated(string message, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		MessagePlayersUntranslatedTimed(message, DAYZBR_MSG_TIME, param1, param2, param3, param4, param5);
	}

	/*
	 * Send a message to all players, with custom message duration
	 * @param message The message to send
	 * @param time The time to display the message for
	 */
	void MessagePlayersUntranslatedTimed(string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>( message, time, param1, param2, param3, param4, param5 ), true);
	}

    /*
	 * Send a message to a specific player, with standard message duration
	 * @param player The player to send the message to
	 * @param message The message to send
	 */
	void MessagePlayerUntranslated(PlayerBase player, string message, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
		MessagePlayerUntranslatedTimed(player, message, DAYZBR_MSG_TIME, param1, param2, param3, param4, param5);
	}

	/*
	 * Send a message to a specific player, with custom message duration
	 * @param player The player to send the message to
	 * @param message The message to send
	 * @param time The time to display the message for
	 */
	void MessagePlayerUntranslatedTimed(PlayerBase player, string message, float time = DAYZBR_MSG_TIME, string param1 = "", string param2 = "", string param3 = "", string param4 = "", string param5 = "")
	{
	    if(player)
	    {
	    	if(player.GetIdentity())
	    	{
	    		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "NotificationMessage", new Param7<string, float, string, string, string, string, string>( message, time, param1, param2, param3, param4, param5 ), true, player.GetIdentity());
	    	}
	    }
	}

    protected bool IsSafeForTeleport(float x, float y, float z, bool check_zone = true)
    {
        BattleRoyaleSpawnsData m_SpawnsSettings = BattleRoyaleConfig.GetConfig().GetSpawnsData();
    	// Check if in zone (if needed)
        if(check_zone && m_SpawnsSettings.spawn_in_first_zone)
        {
            if(!BattleRoyaleZone.GetZone(i_StartingZone).IsInZone(x, z))
                return false;
        }

        // Avoid the sea
        if(GetGame().SurfaceIsSea(x, z))
            return false;

		// Avoid the ponds
        if(GetGame().SurfaceIsPond(x, z))
            return false;

		// Avoid the roads
//        if(GetGame().SurfaceRoadY(x, z) != y)
//            return false;

		// Avoid namalsk ice (and others)
        ref array<string> bad_surface_types = {
            "nam_seaice",
            "nam_lakeice_ext"
        };

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);
        if(bad_surface_types.Find(surface_type) != -1)
            return false;

		// Avoid objects
        vector position;
        position[0] = x;
        position[1] = y;
        position[2] = z;

        vector start = position + Vector( 0, 10, 0 );
        vector end = position;
        float radius = 2.5;

        PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
        Object m_HitObject;
        vector m_HitPosition;
        vector m_HitNormal;
        float m_HitFraction;
        bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

        if(m_Hit)
            return false;

        // New Geometry check
        array<Object> excludedObjects = new array<Object>();
        array<Object> collidedObjects = new array<Object>();
        vector box_position;
        box_position[0] = position[0];
        box_position[1] = position[1];
        box_position[2] = position[2];
        if( GetGame().IsBoxCollidingGeometry(box_position, "0 0 0", "2 10 2", ObjIntersectFire, ObjIntersectGeom, excludedObjects, collidedObjects) )
        {
            if( collidedObjects.Count() > 0)
            {
                BattleRoyaleUtils.Trace("New IsSafeForTeleport Geometry check is true !");
#ifdef BR_TRACE_ENABLED
                Print( collidedObjects );
#endif
                for (int i = 0; i < collidedObjects.Count(); ++i)
                {
                    string objectClass = collidedObjects.Get(i).GetType();
                    BattleRoyaleUtils.Trace( "objectClass: " + objectClass );
                }

                string text = "";
                foreach (Object object: collidedObjects)
                    text += " | " + Object.GetDebugName(object);
            }

			return false;
        }

        return true;
    }

#ifdef Carim
	/*
	 * Get the number of groups in the game
	 * Quickly iterate over the parties and count the number of groups
	 * @return The number of groups
	 */
	int GetGroupsCount()
	{
		ref map<string, ref CarimSet> parties = MissionServer.Cast(GetGame().GetMission()).carimModelPartyParties.mutuals;

		set<string> already_found_players = new set<string>;  // Already found players
		set<string> groups = new set<string>;  // Final groups list

		foreach (string key, ref CarimSet party : parties)
		{
			if (already_found_players.Find(key) != -1)
			{
				continue;  // Skip if already found
			}

			// Add the party to the list of groups
			if (groups.Find(key) == -1)  // Should not be needed
			{
				groups.Insert(key);
			}

			// Add the players to the list of already found players
			foreach (string player : party.ToArray())
			{
				if (already_found_players.Find(player) == -1)
				{
					already_found_players.Insert(player);
				}
			}
		}

		return groups.Count();
	}

    ref array<ref set<PlayerBase>> GetGroups()
    {
        int i;
        ref array<ref set<PlayerBase>> groups = new array<ref set<PlayerBase>>;
        ref array<PlayerBase> m_PlayerWaitList = new array<PlayerBase>;
        m_PlayerWaitList.Copy( m_Players );
        // Create teleport groups

        // Recuperation des parties
        // Recuperation de la partie du joueur
        // Ajout des membres de la partie dans un groupe
        // Suppression de chaque membre de la liste m_PlayerWaitList

		CarimModelPartyParties parties = MissionServer.Cast(GetGame().GetMission()).carimModelPartyParties;
		if (parties)
		{
			BattleRoyaleUtils.Trace("Parties OK");
			BattleRoyaleUtils.Trace("Current parties: " + parties.Repr());

			// Create map player id <-> player object
			auto id_map = new map<string, PlayerBase>();
			array<Man> players = new array<Man>;
			GetGame().GetPlayers(players);
			for (i = 0; i < players.Count(); ++i)
			{
				PlayerBase player = PlayerBase.Cast(players.Get(i));
				if (player && player.GetIdentity() && player.IsAlive())
				{
					//BattleRoyaleUtils.Trace("Player: " + player.GetIdentity().GetName());
					id_map.Insert(player.GetIdentity().GetId(), player);
				}
			}

			// Iterate over parties
			ref set<PlayerBase> group;
			// Using parties.mutuals here instead of parties.registered because mutuals represents
			// the set of parties with confirmed mutual agreements, which is required for this logic.
			ref map<string, ref CarimSet> registered_parties = parties.mutuals;
			int partyCount = registered_parties.Count();
			BattleRoyaleUtils.Trace("There is " + partyCount + " parties");
			for (i = 0; i < partyCount; ++i)
			{
				group = new ref set<PlayerBase>;
				PlayerBase plr = PlayerBase.Cast(id_map.Get(registered_parties.GetKey(i)));  // Get party leader
				if(plr && plr.GetIdentity())
				{
					BattleRoyaleUtils.Trace("Party leader is " + plr.GetIdentity().GetName());
					if(m_PlayerWaitList.Find(plr) != -1)  // Test if party leader is still in waiting list
					{
						BattleRoyaleUtils.Trace(plr.GetIdentity().GetName() + " is in the wait list");
						m_PlayerWaitList.Remove(m_PlayerWaitList.Find(plr));
						if (plr && plr.GetIdentity() && plr.IsAlive())
						{
							BattleRoyaleUtils.Trace("Add leader " + plr.GetIdentity().GetName() + " to the group");
							group.Insert(plr);  // Insert guild leader into group
							if (registered_parties.GetElement(i))
							{
								array<string, bool> tmpPlayPart = registered_parties.GetElement(i).ToArray();  // Get party members
								int tmpPlayPartCount = tmpPlayPart.Count();
								BattleRoyaleUtils.Trace("Party have " + tmpPlayPartCount + " more members");
								for(int j = 0; j < tmpPlayPartCount; j++)  // Iterate over party members
								{
									PlayerBase plrpart = id_map.Get(tmpPlayPart.Get(j));
									if ( plrpart )
									{
										BattleRoyaleUtils.Trace("Try to add player " + plrpart.GetIdentity().GetName() + " to teleport group");
										if(m_PlayerWaitList.Find(plrpart) != -1)
										{
											BattleRoyaleUtils.Trace("Added player " + plrpart.GetIdentity().GetName() + " to teleport group");
											m_PlayerWaitList.Remove(m_PlayerWaitList.Find(plrpart));
											group.Insert(plrpart);  // Insert party member into group
										}
									}
								}
							}
							groups.Insert(group);  // Insert group into list of groups
						}
					} else {
						BattleRoyaleUtils.Trace("Party leader is not in waiting list, do nothing");
					}

					if(m_PlayerWaitList.Count() <= 0)
					{
						BattleRoyaleUtils.Trace("No more players in waiting list, skip the remaining players");
						break;
					}
				}
			}
		}

        // Add remaining players
        ref set<PlayerBase> solo_group;
        int pRemCount = m_PlayerWaitList.Count();
        //BattleRoyaleUtils.Trace("Remaining players: " + pRemCount);
        for (i = 0; i < pRemCount; i++)
        {
            //BattleRoyaleUtils.Trace("Remaining player: " + m_PlayerWaitList.Get(i).GetName());
            solo_group = new set<PlayerBase>;
            solo_group.Insert(m_PlayerWaitList.Get(i));
            groups.Insert(solo_group);
        }

        return groups;
    }
#endif

	void OnPlayerDisconnected(PlayerBase player)
	{
		if(ContainsPlayer( player ))
		{
			RemovePlayer( player );
		}
		else
		{
			Error("Unknown player disconnected! Not in current state?");
		}

		BattleRoyaleServerData m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();

		if ( m_ServerData.enable_vigrid_api )
		{
			BattleRoyaleUtils.Trace("ScoreWebhook: Sending player score");
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
			ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
			scoreWebhook.Send( br_instance.match_uuid, player.player_steamid, player.GetBRPosition() );
		}
	}

	void OnPlayerKilled( PlayerBase player, Object source )
	{
		if( ContainsPlayer( player ) )
		{
			RemovePlayer( player );
		}
		else
		{
			Error("Unknown player killed! Not in current state?");
		}

		if (!player || !source)
		{
	        BattleRoyaleUtils.Trace("DEBUG: PlayerKilled() player/source does not exist");
	        return;
    	}

		if( player.GetIdentity() )
		{
			map<string, string> json_data = new map<string, string>();
			json_data.Insert( "victim", player.GetIdentity().GetPlainId() );
			json_data.Insert( "victim_position", player.GetPosition().ToString() );
			vector killer_position = "0 0 0";

			BattleRoyaleServerData m_ServerData = BattleRoyaleConfig.GetConfig().GetServerData();
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();

			if ( m_ServerData.enable_vigrid_api )
			{
				BattleRoyaleUtils.Trace("ScoreWebhook: Sending player score");
				ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
				scoreWebhook.Send( br_instance.match_uuid, player.GetIdentity().GetPlainId(), player.GetBRPosition() );
			}

			// Does the source is a carrier and the carrier a Player?
			PlayerBase playerSource = PlayerBase.Cast( EntityAI.Cast( source ).GetHierarchyParent() );
			if (!playerSource)
			{
				// If not, does the source is a Player?
				playerSource = PlayerBase.Cast( source );
			}

			if (player == source)	// deaths not caused by another object (starvation, dehydration)
			{
				// Killed by environmental causes but the the player directly
				json_data.Insert( "killer", "environment" );
			}
			else if ( source.IsInherited(Grenade_Base) || source.IsInherited(LandMineTrap) )
			{
				string killer = "";
				EnScript.GetClassVar(source, "m_ActivatorId", -1, killer);
				json_data.Insert( "killer", killer );
				json_data.Insert( "weapon", source.GetType() );
			}
			else {
				json_data.Insert( "killer_position", source.GetPosition().ToString() );

				if (playerSource)
				{
					json_data.Insert( "killer", playerSource.GetIdentity().GetPlainId() );
					GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", new Param1<int>(1), true, playerSource.GetIdentity(), playerSource);
					if (source.IsWeapon() || source.IsMeleeWeapon())
					{
						json_data.Insert( "weapon", source.GetType() );
						if ( !source.IsMeleeWeapon() )
						{
							json_data.Insert( "distance", vector.Distance( player.GetPosition(), playerSource.GetPosition() ).ToString() );
						}
					}
					else
					{
						json_data.Insert( "weapon", "fist" );
					}
				}
				else
				{
					//rest, Animals, Zombies
					json_data.Insert( "killer", source.GetType() );
				}
			}

			EventWebhook eventWebhook = new EventWebhook( m_ServerData.webhook_jwt_token );
			eventWebhook.Send( br_instance.match_uuid, "player.kill", JsonFileLoader<map<string, string>>.JsonMakeData( json_data ) );
		}
	}
}

// Base state for the Debug Zone.
// This handles healing / godmode / and teleporting
class BattleRoyaleDebugState: BattleRoyaleState
{
    protected vector v_Center;
    protected float f_Radius;
    protected int i_HealTickTime;
    protected ref array<string> a_AllowedOutsideLobby;

    void BattleRoyaleDebugState()
    {
        BattleRoyaleSpawnsData m_SpawnsSettings = BattleRoyaleConfig.GetConfig().GetSpawnsData();
        BattleRoyaleLobbyData mLobbySettings = BattleRoyaleConfig.GetConfig().GetDebugData();

        if(m_SpawnsSettings && mLobbySettings)
        {
            v_Center = m_SpawnsSettings.spawn_point;
            f_Radius = m_SpawnsSettings.radius;
            a_AllowedOutsideLobby = mLobbySettings.allowed_outside_lobby;
        }
        else
        {
            Error("DEBUG SETTINGS IS NULL!");
            GetGame().RequestExit(0);  // Exit the game
        }

        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();

        if(m_GameSettings)
        {
            i_HealTickTime = m_GameSettings.debug_heal_tick_seconds;
        }
        else
        {
            Error("GAME SETTINGS IS NULL");
            i_HealTickTime = DAYZBR_DEBUG_HEAL_TICK;
        }
    }

    override void Activate()
    {
        //Note: this is called AFTER players are added
        b_IsDebug = true;
        super.Activate();
    }

    override string GetName()
    {
        return "Unknown Debug State";
    }

    /*
    override void AddPlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(false); //all players in this state are god mode
            player.Heal();
        }

        super.AddPlayer(player);
    }

    override ref array<PlayerBase> RemoveAllPlayers()
    {
        ref array<PlayerBase> players = super.RemoveAllPlayers();
        foreach(PlayerBase player: players)
        {
            player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
        return players;
    }
    
    override void RemovePlayer(PlayerBase player)
    {
        if(player)
        {
            player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
        super.RemovePlayer(player);
    }
    */

    //--- debug states must lock players into the debug zone & heal them
    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        super.OnPlayerTick(player, timeslice);

		// Try to do only a 2D distance between points and not 3D (to avoid the teleport bug on Takistan)
        vector player_position = player.GetPosition();
        float distance = vector.Distance( Vector( player_position[0], 0, player_position[2] ), Vector( v_Center[0], 0, v_Center[2] ) );

        if(distance > f_Radius && !CanGoOutsideLobby(player))
        {
			vector spawn_pos = "0 0 0";
			spawn_pos[0] = Math.RandomFloatInclusive((v_Center[0] - 5), (v_Center[0] + 5));
			spawn_pos[2] = Math.RandomFloatInclusive((v_Center[2] - 5), (v_Center[2] + 5));
			spawn_pos[1] = GetGame().SurfaceY(spawn_pos[0], spawn_pos[2]);

            player.SetPosition(spawn_pos);

			float dir = Math.RandomFloat(0, 360);
			vector playerDir = vector.YawToVector(dir);
			player.SetDirection( Vector(playerDir[0], 0, playerDir[1]) );
        }

        if(player.time_until_heal <= 0)
        {
            player.time_until_heal = i_HealTickTime;
            player.Heal();
        }
        player.time_until_heal -= timeslice;
    }

    bool CanGoOutsideLobby(PlayerBase player)
    {
        if(!player)
        {
            Error("Null player in CanSpectate");
            return false;
        }
        PlayerIdentity identity = player.GetIdentity();

        if(!identity)
            return false;

        string steamid = identity.GetPlainId();
        if(steamid == "")
        {
            Error("Blank SteamId from identity!");
            return false;
        }

        return (a_AllowedOutsideLobby.Find(steamid) != -1);
    }

    //TODO:
    //DEPRECATED: should pull value from config
    //GOAL: make this protected
    vector GetCenter()
    {
        v_Center[1] = GetGame().SurfaceY(v_Center[0], v_Center[2]);
        return v_Center;
    }

    //DEPRECATED: should pull value from config
    //GOAL: make this protected
    float GetRadius()
    {
        return f_Radius;
    }
}
