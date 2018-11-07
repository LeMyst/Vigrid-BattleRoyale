modded class MissionServer
{
	vector GetDebugPostion()
	{
		if(BR_GAME)
		{
			if(BR_GAME.m_BattleRoyaleData)
			{
				return BR_GAME.m_BattleRoyaleData.debug_position;
			}
		}
		Print("ERROR: BR GAME NOT READY FOR DEBUG ZONE");
		
		return "14829.2 72.3148 14572.3"; //No debug position?
	}
	
	
	ref BattleRoyale BR_GAME;
	
	
	override void OnInit()
	{
		log("ON INIT");
		BR_GAME = new BattleRoyale( NULL );
		BR_GAME.OnInit();
	}
	
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		BR_GAME.OnUpdate(timeslice);
	}
	
	override void OnClientRespawnEvent(PlayerIdentity identity, PlayerBase player)
	{
		super.OnClientRespawnEvent(identity,player);
		
		
		if(player)
		{
			vector playerPos = player.GetPosition();
			float distance = vector.Distance(playerPos,GetDebugPostion());
			
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
	
	
	override void OnClientDisconnectedEvent(PlayerIdentity identity, PlayerBase player, int queueTime, bool authFailed)
	{
		
		BR_GAME.OnPlayerDisconnected(player);
		super.OnClientDisconnectedEvent(identity, player,queueTime,authFailed);
	}
	
	override PlayerBase OnClientNewEvent(PlayerIdentity identity, vector pos, ParamsReadContext ctx)
	{
		PlayerBase plr = super.OnClientNewEvent(identity,pos,ctx);
		if(plr)
		{
			BR_GAME.OnPlayerConnected(plr);
		}
		return plr;
	}
	
	override void OnClientReadyEvent(PlayerIdentity identity, PlayerBase player)
	{
		player.SetPosition(GetDebugPostion());
		super.OnClientReadyEvent(identity,player);
		BR_GAME.OnPlayerConnected(player);
	}
	
	override void HandleBody(PlayerBase player)
	{
		vector playerPos = player.GetPosition();
		float distance = vector.Distance(playerPos,GetDebugPostion());
		if(distance <= 50)
		{
			player.Delete();
		}
		else
		{
			super.HandleBody(player);
		}
	}
}