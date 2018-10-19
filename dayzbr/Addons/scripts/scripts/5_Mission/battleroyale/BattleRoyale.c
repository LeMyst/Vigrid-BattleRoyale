//Global functionality
void SendMessage(PlayerBase target, string msg, bool required = true)
{
	ref Param1<string> value_string = new Param1<string>(msg);
    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SendClientMessage", value_string, required, target.GetIdentity(), target );
}

void SendMessageAll(string msg, bool required = true)
{
	ref Param1<string> value_string = new Param1<string>(msg);
    GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SendGlobalMessage", value_string, required );
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

	ref BattleRoyaleZoneManager m_BattleRoyaleZoneManager;
	
	ref BattleRoyaleRound m_BattleRoyaleRound;
	ref BattleRoyaleRound m_BattleRoyaleRound_2;
	
	ref BattleRoyaleDebug m_BattleRoyaleDebug;
	
	ref ScriptCallQueue br_CallQueue;
	
	bool hasInit;
	
	//NOTE missionserver is passed in the event that we need it in the future (it is unused at this momemnt)
	void BattleRoyale(MissionServer server_class)
	{
		if ( GetGame().IsServer() )
		{
			hasInit = false;
			m_BattleRoyaleData = StaticBRData.LoadDataServer();
			m_BattleRoyaleDebug = new BattleRoyaleDebug(this);
			m_BattleRoyaleZoneManager = new BattleRoyaleZoneManager(m_BattleRoyaleData);

			m_BattleRoyaleRound = new BattleRoyaleRound(m_BattleRoyaleData, m_BattleRoyaleDebug, m_BattleRoyaleZoneManager,"ROUND 1");
			m_BattleRoyaleRound_2 = new BattleRoyaleRound(m_BattleRoyaleData, m_BattleRoyaleDebug, m_BattleRoyaleZoneManager,"ROUND 2");
			
			br_CallQueue = new ScriptCallQueue(); 
		} else
		{
			hasInit = true;
		}
	}
	
	void OnInit()
	{
		if ( !GetGame().IsServer() ) return;
		
		//prevent double inits
		if(!hasInit)
			hasInit = true;
		else
			return;
		
		
		m_BattleRoyaleDebug.Init();
		m_BattleRoyaleRound.Init();
		m_BattleRoyaleRound_2.Init();
		br_CallQueue.CallLater(this.ProcessRoundStart, 5000, true);
	}
	
	
	void OnUpdate(float timespan)
	{
		if ( !GetGame().IsServer() ) return;

		//Run any items on the call queue
		br_CallQueue.Tick(timespan);
		
		//process updates in the round and debug
		m_BattleRoyaleRound.OnUpdate(timespan);
		m_BattleRoyaleDebug.OnUpdate(timespan);
		m_BattleRoyaleRound_2.OnUpdate(timespan);
	}
	
	
	void ProcessRoundStart()
	{
		if ( !GetGame().IsServer() ) return;

		if(!m_BattleRoyaleRound.inProgress)
		{
			if(m_BattleRoyaleDebug.m_DebugPlayers.Count() >= m_BattleRoyaleData.minimum_players)
			{
				BRLOG("DAYZBR: ROUND START CALL");
				m_BattleRoyaleRound.PlayerCountReached();
			}
		}
		else if(!m_BattleRoyaleRound_2.inProgress)
		{
			//if round 1 is in progress, try using round 2
			if(m_BattleRoyaleDebug.m_DebugPlayers.Count() >= m_BattleRoyaleData.minimum_players)
			{
				BRLOG("DAYZBR: ROUND START CALL");
				m_BattleRoyaleRound_2.PlayerCountReached();
			}
		}
	}
	
	//player connected. add them to the debug zone and prep them for BR
	void OnPlayerConnected(PlayerBase player)
	{
		if ( !GetGame().IsServer() ) return;

		BRLOG("DAYZBR: PLAYER CONNECTED");
		
		player.BR_BASE = this; //Register the player for Player based event calls
		
		m_BattleRoyaleDebug.AddPlayer(player);
	}
	
	//player disconnected. remove them from whichever class they are in
	void OnPlayerDisconnected(PlayerBase player)
	{
		if ( !GetGame().IsServer() ) return;

		BRLOG("DAYZBR: PLAYER DISCONNECTED");
		
		m_BattleRoyaleRound.RemovePlayer(player);
		m_BattleRoyaleRound_2.RemovePlayer(player);
		m_BattleRoyaleDebug.RemovePlayer(player);
		
	}
	
	override bool allowFallDamage(PlayerBase plr)
	{
		if(m_BattleRoyaleRound.ContainsPlayer(plr))
			return m_BattleRoyaleRound.RoundStarted; //if round has started, allow it, else, do not.
		
		if(m_BattleRoyaleRound_2.ContainsPlayer(plr))
			return m_BattleRoyaleRound_2.RoundStarted; //if round 2 has started, allow it, else, do not.
		
		return true;
	}
	//player tick. process them for which class they are in
	override void OnPlayerTick(PlayerBase player, float ticktime)
	{
		if ( !GetGame().IsServer() ) return;

		//on player tick
		if(m_BattleRoyaleDebug.ContainsPlayer(player))
		{
			m_BattleRoyaleDebug.OnPlayerTick(player,ticktime);
		}
		else if(m_BattleRoyaleRound.ContainsPlayer(player))
		{
			m_BattleRoyaleRound.OnPlayerTick(player,ticktime);
		}
		else if(m_BattleRoyaleRound_2.ContainsPlayer(player))
		{
			m_BattleRoyaleRound_2.OnPlayerTick(player, ticktime);
		}
		else
		{
			//TODO: process clients that are bugged (they could also be between states)
		}
	}

	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		if ( !GetGame().IsServer() ) return;

		// on player killed
		if(m_BattleRoyaleDebug.ContainsPlayer(killed))
		{
			m_BattleRoyaleDebug.OnPlayerKilled(killed,killer);
		}
		else if(m_BattleRoyaleRound.ContainsPlayer(killed))
		{
			m_BattleRoyaleRound.OnPlayerKilled(killed,killer);
		}
		else if(m_BattleRoyaleRound_2.ContainsPlayer(killed))
		{
			m_BattleRoyaleRound_2.OnPlayerKilled(killed,killer);
		}
		else
		{
			//TODO: client died but was bugged?
		}
	}
}



