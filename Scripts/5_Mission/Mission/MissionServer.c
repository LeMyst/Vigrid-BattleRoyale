modded class MissionServer
{
	
	//--- TODO: look at dayzexpansion missionserver
	//TODO: look at dayz missionserver
	//TODO: look at old BR missionserver
	
	override void OnInit()
	{
		super.OnInit();
		//TODO: br things
		m_BattleRoyale = new BattleRoyaleServer;
		
	}
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		//TODO: br things
		BattleRoyaleServer.Cast( m_BattleRoyale ).Update(timeslice);
	}
	override void InvokeOnConnect( PlayerBase player, PlayerIdentity identity )
	{
		super.InvokeOnConnect(player, identity);
		//TODO: player connected to server, handle
		if(player)
		{
			BattleRoyaleServer.Cast( m_BattleRoyale ).OnPlayerConnected(player);
		}
		
	}
	override void InvokeOnDisconnect( PlayerBase player )
	{
		super.InvokeOnDisconnect(player);
		//TODO: player disconnected, handle
		if(player)
		{
			player.SetHealth("", "", 0.0); //don't let this character get saved in the hive
			BattleRoyaleServer.Cast( m_BattleRoyale ).OnPlayerDisconnected(player);
		}
		
		
	}
	
	
	


	//--- Functional overrides
	override void HandleBody(PlayerBase player)
	{
		//if body in debug zone, then delete it, else call super handlebody
		if(player)
		{
			vector playerPos = player.GetPosition();
			float distance = vector.Distance(playerPos,BattleRoyaleServer.Cast( m_BattleRoyale ).GetDebug().GetCenter());
			if(distance <= BattleRoyaleServer.Cast( m_BattleRoyale ).GetDebug().GetRadius())
			{
				player.Delete();
				return;
			}
		}
		super.HandleBody( player );
	}
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		vector debug_pos = BattleRoyaleServer.Cast( m_BattleRoyale ).GetDebug().GetCenter(); //TODO: we need debug zone position
		return super.OnClientNewEvent(identity, debug_pos, ctx);
	}
	override void StartingEquipSetup(PlayerBase player, bool clothesChosen)
	{
		//TODO: figure out how to set inventory loadout (we'll hard code BR specific starting items here
		//EntityAI item = m_player.GetInventory().CreateInInventory(topsArray.GetRandomElement());
	}
	override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientRespawnEvent(identity, player);
		
		if(player)
		{
			//if player is inside the debug zone, delete the body (to prevent debug body spam)
			vector playerPos = player.GetPosition();
			float distance = vector.Distance(playerPos,BattleRoyaleServer.Cast( m_BattleRoyale ).GetDebug().GetCenter());
			if(distance <= BattleRoyaleServer.Cast( m_BattleRoyale ).GetDebug().GetRadius())
			{
				player.Delete();
			}
		}
		
		
	}
}