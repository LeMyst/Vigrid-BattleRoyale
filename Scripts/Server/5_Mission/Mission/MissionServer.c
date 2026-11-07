#ifdef SERVER
modded class MissionServer
{
	//--- TODO: look at dayzexpansion missionserver
	//TODO: look at dayz missionserver
	//TODO: look at old BR missionserver
	override void OnInit()
	{
		BattleRoyaleUtils.Trace("Vigrid-BattleRoyale OnInit()");
		super.OnInit();

		// br things
		m_BattleRoyale = new BattleRoyaleServer;
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		BattleRoyaleServer.Cast( m_BattleRoyale ).Update(timeslice);

		//--- After the match server's own Update, never inside it: the in-place reset replaces that
		//--- object, which must not happen from one of its own frames. Two integer tests when idle.
		BattleRoyaleInPlaceReset.Tick( this );
	}

	/**
	 *  In-place reset: replace the match server with a fresh one, whose Init does everything a boot
	 *  does (new match id, new circles, unlock, lobby). Called only from BattleRoyaleInPlaceReset,
	 *  i.e. from OnUpdate above and never from inside the old server's Update.
	 *
	 *  One assignment, so m_BattleRoyale is never NULL: every method in this class dereferences it
	 *  without a check. Assigning the field releases the old server, its states and their timers.
	 */
	BattleRoyaleServer BR_ReplaceServer()
	{
		BattleRoyaleServer fresh = new BattleRoyaleServer;
		m_BattleRoyale = fresh;
		return fresh;
	}

	/**
	 *  In-place reset: give a still-connected player a fresh body in the lobby, exactly as a new
	 *  connection gets one - the two lines of vanilla CreateCharacter, then EquipCharacter and
	 *  InvokeOnConnect, which places them in the lobby, adds them to it and lets Party and Map
	 *  greet them. The same CreatePlayer + SelectPlayer pair BattleRoyaleSpectators.AdminRespawn
	 *  uses on a dead identity. The caller deletes the old body afterwards.
	 */
	PlayerBase BR_SpawnLobbyBody(PlayerIdentity identity, string character_type)
	{
		if ( !identity )
			return NULL;

		if ( character_type == "" )
			character_type = g_Game.CreateRandomPlayer();

		BattleRoyaleServer server = BattleRoyaleServer.Cast( m_BattleRoyale );
		vector position = "0 0 0";
		server.GetAdminSpawnPosition( position );

		PlayerBase body = PlayerBase.Cast( GetGame().CreatePlayer( identity, character_type, position, 0, "NONE" ) );
		if ( !body )
			return NULL;

		GetGame().SelectPlayer( identity, body );

		m_player = body;
		EquipCharacter( NULL );

		InvokeOnConnect( body, identity );
		return body;
	}

	override void InvokeOnConnect( PlayerBase player, PlayerIdentity identity )
	{
		BattleRoyaleUtils.Trace("InvokeOnConnect()");
		super.InvokeOnConnect(player, identity);

		if(player)
		{
			BattleRoyaleServer.Cast( m_BattleRoyale ).OnPlayerConnected(player); //this never ran?
		}
		else
		{
			Error("PLAYER PASSED TO IOC IS NULL");
		}
	}

	override void OnEvent(EventType eventTypeId, Param params)
	{
		switch (eventTypeId)
		{
		case ClientPrepareEventTypeID:
			BattleRoyaleUtils.Trace("ClientPrepareEventTypeID");
			break;

		case ClientNewEventTypeID:
			BattleRoyaleUtils.Trace("ClientNewEventTypeID");
			break;

		case ClientReadyEventTypeID:
			BattleRoyaleUtils.Trace("ClientReadyEventTypeID");
			break;

		case ClientRespawnEventTypeID:
			BattleRoyaleUtils.Trace("ClientRespawnEventTypeID");
			break;

		case ClientReconnectEventTypeID:
			BattleRoyaleUtils.Trace("ClientReconnectEventTypeID");
			break;

		case ClientDisconnectedEventTypeID:
			BattleRoyaleUtils.Trace("ClientDisconnectedEventTypeID");
			break;

		case LogoutCancelEventTypeID:
			BattleRoyaleUtils.Trace("LogoutCancelEventTypeID");
			break;
		}
		super.OnEvent(eventTypeId, params);
	}

	override void EquipCharacter(MenuDefaultCharacterData char_data)
	{
		BattleRoyaleLobbyData m_LobbySettings = BattleRoyaleConfig.GetConfig().GetLobbyData();

		array<string> player_lobby_items = m_LobbySettings.player_lobby_items;
		array<int> player_lobby_items_shortcut = m_LobbySettings.player_lobby_items_shortcut;

		for(int i = 0; i < player_lobby_items.Count(); i++)
		{
			string item_name = player_lobby_items.Get(i);
			EntityAI item = m_player.GetInventory().CreateInInventory(item_name);
			if(item)
			{
				// set health to the maximum
				item.SetHealth(item.GetMaxHealth());

				//see if this item has a shortcut
				int shortcut_index = player_lobby_items_shortcut.Find(i);
				if(shortcut_index != -1)
				{
					m_player.SetQuickBarEntityShortcut(item, shortcut_index);
				}
			}
			else
			{
				BattleRoyaleUtils.Trace("Failed to create lobby item: " + item_name);
			}
		}

		StartingEquipSetup(m_player, true);
	}

	override void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
	{
		if(player)
		{
			BattleRoyaleServer.Cast( m_BattleRoyale ).OnPlayerDisconnected(player, identity);
		}
		super.PlayerDisconnected( player, identity, uid );
	}

	//--- Functional overrides
	override void HandleBody(PlayerBase player)
	{
		//if body in debug zone, then delete it, else call super handlebody
		if(player)
		{
			BattleRoyaleDebug m_Debug;
			BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( m_BattleRoyale ).GetCurrentState();
			if(Class.CastTo(m_Debug, m_CurrentState))
			{
				//we are currently in the debug state
				vector playerPos = player.GetPosition();
				float distance = vector.Distance(playerPos,m_Debug.GetCenter());
				if(distance <= m_Debug.GetRadius())
				{
					player.Delete();
					return;
				}
			}
		}

		super.HandleBody( player );
	}

	override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientRespawnEvent(identity, player);

		//this should never happen, but maybe it could if in the debug zone so lets handle that case
		if( player )
		{
			//if player is inside the debug zone, delete the body (to prevent debug body spam)
			BattleRoyaleDebug m_Debug;
			BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( m_BattleRoyale ).GetCurrentState();
			if( Class.CastTo( m_Debug, m_CurrentState ) )
			{
				//we are currently in the debug state
				vector playerPos = player.GetPosition();
				float distance = vector.Distance(playerPos,m_Debug.GetCenter());
				if(distance <= m_Debug.GetRadius())
				{
					player.Delete();
				}
			}
			else
			{
				//can't cast current state to debug? Kick
				BattleRoyaleUtils.Trace("Kicking player (Not in debug state)");
				GetGame().DisconnectPlayer( identity );
				// TODO: Replace with RPC call to ask the player to disconnect
			}
		}
		else
		{
			//Really no idea what this could be... maybe dead? Kick
			BattleRoyaleUtils.Trace("Kicking player (Not a valid player object)");
			GetGame().DisconnectPlayer( identity );
			// TODO: Replace with RPC call to ask the player to disconnect
		}
	}

	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int logoutTime, bool authFailed)
	{
		if(player)
		{
			BattleRoyaleServer.Cast( m_BattleRoyale ).OnPlayerDisconnect(player, identity);
		}

		super.OnClientDisconnectedEvent(identity, player, logoutTime, authFailed);
	}
}
#endif
