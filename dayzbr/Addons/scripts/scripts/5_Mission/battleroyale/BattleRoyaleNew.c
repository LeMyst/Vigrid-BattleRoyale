//Global functionality
void SendMessage(PlayerBase target, string msg, bool required = true)
{
	ref Param1<string> value_string = new Param1<string>(msg);
	GetGame().RPCSingleParam(target,MRPCs.RPC_BR_SEND_CLIENT_MSG,value_string,required,target.GetIdentity());
	
}
void SendMessageAll(string msg, bool required = true)
{
	ref Param1<string> value_string = new Param1<string>(msg);
	GetGame().RPCSingleParam(NULL,MRPCs.RPC_BR_SEND_GLOBAL_MSG,value_string,required,NULL);
}
void BRLOG(string msg)
{
	Print("=============DayZBR LOG================");
	Print("[Log]: " + msg);
	Print("=============DayZBR LOG================");
	Debug.Log("=============DayZBR LOG================");
	Debug.Log(msg);
	Debug.Log("=============DayZBR LOG================");
}
class BattleRoyale extends BattleRoyaleBase
{
	ref StaticBRData m_BattleRoyaleData;
	
	ref BattleRoyaleRound m_BattleRoyaleRound;
	ref BattleRoyaleDebug m_BattleRoyaleDebug;
	
	ref ScriptCallQueue br_CallQueue;
	
	bool hasInit;
	
	//NOTE missionserver is passed in the event that we need it in the future (it is unused at this momemnt)
	void BattleRoyale(MissionServer server_class)
	{
		hasInit = false;
		m_BattleRoyaleData = StaticBRData.LoadDataServer();
		
		m_BattleRoyaleRound = new BattleRoyaleRound(this);
		m_BattleRoyaleDebug = new BattleRoyaleDebug(this);
		
		br_CallQueue = new ScriptCallQueue(); 
	}
	
	void OnInit()
	{
		//prevent double inits
		if(!hasInit)
			hasInit = true;
		else
			return;
		
		
		m_BattleRoyaleDebug.Init();
		br_CallQueue.CallLater(this.ProcessRoundStart, 5000, true);
	}
	
	
	void OnUpdate(float timespan)
	{
		//Run any items on the call queue
		br_CallQueue.Tick(timespan);
		
		//process updates in the round and debug
		m_BattleRoyaleRound.OnUpdate(timespan);
		m_BattleRoyaleDebug.OnUpdate(timespan);
	}
	
	
	void ProcessRoundStart()
	{
		if(!m_BattleRoyaleRound.inProgress)
		{
			if(m_BattleRoyaleDebug.m_DebugPlayers.Count() >= m_BattleRoyaleData.minimum_players)
			{
				BRLOG("DAYZBR: ROUND START CALL");
				m_BattleRoyaleRound.PlayerCountReached();
			}
		}
	}
	
	//player connected. add them to the debug zone and prep them for BR
	void OnPlayerConnected(PlayerBase player)
	{
		BRLOG("DAYZBR: PLAYER CONNECTED");
		
		player.BR_BASE = this; //Register the player for Player based event calls
		
		m_BattleRoyaleDebug.AddPlayer(player);
		
	}
	
	//player disconnected. remove them from whichever class they are in
	void OnPlayerDisconnected(PlayerBase player)
	{
		BRLOG("DAYZBR: PLAYER DISCONNECTED");
		
		m_BattleRoyaleRound.RemovePlayer(player);
		m_BattleRoyaleDebug.RemovePlayer(player);
		
	}
	
	
	//player tick. process them for which class they are in
	override void OnPlayerTick(PlayerBase player, float ticktime)
	{
		//on player tick
		if(m_BattleRoyaleDebug.ContainsPlayer(player))
		{
			m_BattleRoyaleDebug.OnPlayerTick(player,ticktime);
		}
		else if(m_BattleRoyaleRound.ContainsPlayer(player))
		{
			m_BattleRoyaleRound.OnPlayerTick(player,ticktime);
		}
		else
		{
			//TODO: process clients that are bugged (they could also be between states)
		}
		
	}
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		// on player killed
		if(m_BattleRoyaleDebug.ContainsPlayer(killed))
		{
			m_BattleRoyaleDebug.OnPlayerKilled(killed,killer);
		}
		else if(m_BattleRoyaleRound.ContainsPlayer(killed))
		{
			m_BattleRoyaleRound.OnPlayerKilled(killed,killer);
		}
		else
		{
			//TODO: client died but was bugged?
		}
	}
}



