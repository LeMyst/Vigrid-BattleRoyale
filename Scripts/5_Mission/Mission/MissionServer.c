#define BR_BETA_LOGGING

modded class MissionServer
{
	
	//--- TODO: look at dayzexpansion missionserver
	//TODO: look at dayz missionserver
	//TODO: look at old BR missionserver
	
	override void OnInit()
	{
		super.OnInit();
		//TODO: br things
		Print("INITIALIZING BATTLE ROYALE SERVER");
		m_BattleRoyale = new BattleRoyaleServer;
		
	}
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		
		BattleRoyaleServer.Cast( m_BattleRoyale ).Update(timeslice);
	}
	override void InvokeOnConnect( PlayerBase player, PlayerIdentity identity )
	{
		BRPrint("InvokeOnConnect()")
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
	override void InvokeOnDisconnect( PlayerBase player )
	{
		super.InvokeOnDisconnect(player);
		
		BRPrint("InvokeOnDisconnect()");

		if(player)
		{
			BattleRoyaleServer.Cast( m_BattleRoyale ).OnPlayerDisconnected(player);
		}
		else
		{
			Error("PLAYER PASSED TO IOD IS NULL");
		}
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
	
	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		//TODO: figure out how to set inventory loadout (we'll hard code BR specific starting items here
		//EntityAI item = m_player.GetInventory().CreateInInventory(topsArray.GetRandomElement());
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
		}
		
		
	}
}