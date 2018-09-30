modded class MissionServer
{
	//TODO:Replace all instances where this is used with BattleRoyale.debug_position
	vector debug_position = "14829.2 72.3148 14572.3";
	
	
	ref BattleRoyale BR_GAME;
	
	
	override void OnInit()
	{
		log("ON INIT");
		BR_GAME = new BattleRoyale(this);
		BR_GAME.OnInit();
	}
	
	//TODO: connect the BattleRoyale class OnUpdate here
	override void OnUpdate(float timeslice)
	{
		UpdateDummyScheduler();
		TickScheduler(timeslice);
		UpdateLogoutPlayers();	
		BR_GAME.OnUpdate(timeslice);
	}

	
	
	
	void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
	{
		// note: player is now killed in db right after the actual kill happens 
		/*if (GetHive() && player)
		{
			GetHive().CharacterKill(player);
		}*/
		if(player)
		{
			if (player.IsUnconscious() || player.IsRestrained())
			{
				// kill character
				player.SetHealth("", "", 0.0);
			}
			vector playerPos = player.GetPosition();
			float distance = vector.Distance(playerPos,debug_position);
			
			//TODO: calculate if body is in debug zone, if so, delete
			if(distance <= 50)
			{
				player.Delete();
			}
		}
		
		
		
		
	}
	
	
	
	void log(string msg)
	{
		Print("=============DayZBR LOG================");
		Print("[Log]: " + msg);
		Print("=============DayZBR LOG================");
		
		Debug.Log("=============DayZBR LOG================");
		Debug.Log(msg);
		Debug.Log("=============DayZBR LOG================");
	}
	
	void UpdateLogoutPlayers()
	{
		for ( int i = 0; i < m_LogoutPlayers.Count(); i++ )
		{
			LogoutInfo info = m_LogoutPlayers.GetElement(i);
			
			if (GetGame().GetTime() >= info.param1)
			{
				PlayerIdentity identity;
				PlayerBase player = m_LogoutPlayers.GetKey(i);
				if (player)
				{
					identity = player.GetIdentity();
				}
				PlayerDisconnected(player, identity, info.param2);
					
				m_LogoutPlayers.RemoveElement(i);
				i--;
			}
		}
	}
	void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int queueTime, bool authFailed)
	{
		BR_GAME.OnPlayerDisconnected(player);
		// TODO: get out of vehicle
		// using database and no saving if authorization failed
		if (GetHive() && !authFailed && queueTime > 0)
		{
			// player alive
			if (player.GetPlayerState() == EPlayerStates.ALIVE && !player.IsRestrained())
			{
				if (!m_LogoutPlayers.Contains(player))
				{
					// wait for some time before logout and save
					LogoutInfo params = new LogoutInfo(GetGame().GetTime() + queueTime * 1000, identity.GetId());
					m_LogoutPlayers.Insert(player, params);
					Print("[Logout]: New player " + identity.GetId() + " with logout time " + queueTime.ToString());
				}	
				log("On Client Disconnect - Queue for logout");				
				return;
			}
		}
		PlayerDisconnected(player, identity, identity.GetId());
	}
	PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		log("On Client New");
		
		string characterName;
		// get login data for new character
		// note: ctx can be filled on client in StoreLoginData()
		ProcessLoginData(ctx, m_top, m_bottom, m_shoes, m_skin);
		
		if (m_top == -1 || m_bottom == -1 || m_shoes == -1 || m_skin == -1)
		{
			characterName = GetGame().CreateRandomPlayer();
		}
		else
		{
			characterName = GetGame().ListAvailableCharacters().Get(m_skin);
		}
		
		if (CreateCharacter(identity, debug_position, ctx, characterName))
		{
			BR_GAME.OnPlayerConnected(m_player);
			EquipCharacter();
			GetGame().RPCSingleParam(m_player, ERPCs.RPC_CHARACTER_EQUIPPED, NULL, true, m_player.GetIdentity());
		}
		if(m_player != null)
		{
			log("Forced debug spawn");
			m_player.SetPosition(debug_position);
			//TODO: give m_player god mode
		}
		
		return m_player;
	}
	
	override void OnEvent(EventType eventTypeId, Param params) 
	{
		PlayerIdentity identity;
		PlayerBase player;
		int counter = 0;
		
		switch(eventTypeId)
		{
		case PreloadEventTypeID:
			PreloadEventParams preloadParams;
			Class.CastTo(preloadParams, params);
			
			OnPreloadEvent(preloadParams.param1, preloadParams.param2, preloadParams.param3, preloadParams.param4, preloadParams.param5);
			break;

		case ClientNewEventTypeID:
			ClientNewEventParams newParams;
			Class.CastTo(newParams, params);
			
			player = OnClientNewEvent(newParams.param1, newParams.param2, newParams.param3);
			if (!player)
			{
				Debug.Log("ClientNewEvent: Player is empty");
				return;
			}
			identity = newParams.param1;
			InvokeOnConnect(player,identity );
			break;
			
		case ClientReadyEventTypeID:
			ClientReadyEventParams readyParams;
			Class.CastTo(readyParams, params);
			
			identity = readyParams.param1;
			Class.CastTo(player, readyParams.param2);
			if (!player)
			{
				Debug.Log("ClientReadyEvent: Player is empty");
				return;
			}
			
			OnClientReadyEvent(identity, player);
			InvokeOnConnect(player, identity);
			break;
					
		case ClientRespawnEventTypeID:
			ClientRespawnEventParams respawnParams;
			Class.CastTo(respawnParams, params);
			
			identity = respawnParams.param1;
			Class.CastTo(player, respawnParams.param2);
			if (!player)
			{
				Debug.Log("ClientRespawnEvent: Player is empty");
				return;
			}
			
			OnClientRespawnEvent(identity, player);
			break;
		
		case ClientDisconnectedEventTypeID:
			ClientDisconnectedEventParams discoParams;
			Class.CastTo(discoParams, params);		
			
			identity = discoParams.param1;
			Class.CastTo(player, discoParams.param2);			
			int discTime = discoParams.param3;
			bool authFailed = discoParams.param4;

			if (!player)
			{
				Debug.Log("ClientDisconnectenEvent: Player is empty");
				return;
			}
						
			InvokeOnDisconnect(player);
			OnClientDisconnectedEvent(identity, player, discTime, authFailed);	
			break;
			
		case LogoutCancelEventTypeID:
			LogoutCancelEventParams logoutCancelParams;
				
			Class.CastTo(logoutCancelParams, params);				
			Class.CastTo(player, logoutCancelParams.param1);
			
			identity = player.GetIdentity();
			if (identity)
			{
				Print("[Logout]: Player " + identity.GetId() + " cancelled"); 
			}
			else
			{
				Print("[Logout]: Player cancelled"); 
			}
			m_LogoutPlayers.Remove(player);
			break;
		}
	}
	
	//NOTE: This should never fire, any player that disconnects should be killed if they are not in debug zone. If it does, we teleport them back to debug
	void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		log("Client Ready Event");
		player.SetPosition(debug_position);
		GetGame().SelectPlayer(identity, player);
		BR_GAME.OnPlayerConnected(player);
	}
	
	//NOTE: This should fire every time a player joins. Their "spawn position" is overriden with the debug zone location
	PlayerBase CreateCharacter(PlayerIdentity identity, vector pos, ParamsReadContext ctx, string characterName)
	{		
		log("Create Character");
		Entity playerEnt;
		playerEnt = GetGame().CreatePlayer(identity, characterName, pos, 5, "NONE");//Creates random player
		Class.CastTo(m_player, playerEnt);
		
		GetGame().SelectPlayer(identity, m_player);
		
		//m_player.SetPosition(debug_position); 
		
		return m_player;
		//moduleDefaultCharacter.FileDelete(moduleDefaultCharacter.GetFileName());
	}

	void PlayerDisconnected(PlayerBase player, PlayerIdentity identity, string uid)
	{
		log("Player Disconnected");
		// Note: At this point, identity can be already deleted
		if (!player)
		{
			Print("[Logout]: Skipping player " + uid + ", already removed");
			return;
		}
		
		Print("[Logout]: Player " + uid + " finished");

		//If we have a hive connection, we need to ignore it
		if (GetHive())
		{
			log("Hive found");
			//NOTE: this kills the disconnected player in the DB. 
			GetHive().CharacterKill(player);
			
			// save player
			//player.Save();
			
			// unlock player in DB	
			GetHive().CharacterExit(player);		
		}
		
		//handle the body (debug zone bodies get deleted, others get killed)
		HandleBody(player);
		
		// remove player from server
		GetGame().DisconnectPlayer(identity, uid);
	}
	
	
	
	//TODO: Update this so players in the debug zone have their body deleted
	void HandleBody(PlayerBase player)
	{
		log("Handle Body");
		
		vector playerPos = player.GetPosition();
		float distance = vector.Distance(playerPos,debug_position);
		
		//TODO: calculate if body is in debug zone, if so, delete
		if(distance <= 50)
		{
			player.Delete();
		}
		else
		{
			player.SetHealth("", "", 0.0); //anyone that disconnects while alive has their body killed off
		}
	}
}