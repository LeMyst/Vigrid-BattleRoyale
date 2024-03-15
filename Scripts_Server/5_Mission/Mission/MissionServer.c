#ifdef SERVER
modded class MissionServer
{
	//--- TODO: look at dayzexpansion missionserver
	//TODO: look at dayz missionserver
	//TODO: look at old BR missionserver
	override void OnInit()
	{
		Print("Vigrid-BattleRoyale OnInit()");
		super.OnInit();
		// br things
		m_BattleRoyale = new BattleRoyaleServer;
	}

	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		BattleRoyaleServer.Cast( m_BattleRoyale ).Update(timeslice);
	}

	override void InvokeOnConnect( PlayerBase player, PlayerIdentity identity )
	{
		Print("InvokeOnConnect()");
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
		//TODO: get this from a setting file
		m_player.GetInventory().CreateInInventory( "TShirt_DBR" );
		m_player.GetInventory().CreateInInventory( "Jeans_Black" );
		m_player.GetInventory().CreateInInventory( "Sneakers_Black" );

		m_player.GetInventory().CreateInInventory( "Apple" );
		EntityAI new_item = m_player.GetInventory().CreateInInventory( "Zucchini" );
		m_player.SetQuickBarEntityShortcut(new_item, 0);
		m_player.GetInventory().CreateInInventory( "Apple" );

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
		if(player)
		{
			//if player is inside the debug zone, delete the body (to prevent debug body spam)
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
				}
			}
			else
			{
				//can't cast current state to debug? Kick
				if(!BattleRoyaleServer.Cast( m_BattleRoyale ).GetSpectatorSystem().CanIdSpectate( identity ))
				{
					Print("Kicking player (Not in debug state | Not a spectator)");
					GetGame().DisconnectPlayer( identity );
				}
			}
		}
		else
		{
			//Really no idea what this could be... maybe dead? Kick
			if(!BattleRoyaleServer.Cast( m_BattleRoyale ).GetSpectatorSystem().CanIdSpectate( identity ))
			{
				Print("Kicking player (Not a valid player object | Not a spectator)");
				GetGame().DisconnectPlayer( identity );
			}
		}
	}
}
#endif
