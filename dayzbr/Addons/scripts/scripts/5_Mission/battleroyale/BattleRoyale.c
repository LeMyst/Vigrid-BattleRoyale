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
	
	ref array<BattleRoyaleRound> m_BattleRoyaleRounds;
	
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

			
			m_BattleRoyaleRounds = new array<BattleRoyaleRound>();
			for(int i = 1; i <= m_BattleRoyaleData.num_parallel_matches;i++)
			{
				m_BattleRoyaleRounds.Insert(new BattleRoyaleRound(m_BattleRoyaleData, m_BattleRoyaleDebug, m_BattleRoyaleZoneManager,"ROUND " + i.ToString()));
			}
			BRLOG("Added " + m_BattleRoyaleData.num_parallel_matches.ToString() + " rounds");
			
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
		
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			round.Init();
		}
		
		br_CallQueue.CallLater(this.ProcessRoundStart, 5000, true);
	}
	
	
	void OnUpdate(float timespan)
	{
		if ( !GetGame().IsServer() ) return;

		//Run any items on the call queue
		br_CallQueue.Tick(timespan);
		
		//process updates in the round and debug
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			round.OnUpdate(timespan);
		}
		m_BattleRoyaleDebug.OnUpdate(timespan);
	}
	
	
	void ProcessRoundStart()
	{
		//sum ting wong
		if ( !GetGame().IsServer() ) return;
		
		//not enough players
		if( m_BattleRoyaleDebug.m_DebugPlayers.Count() < m_BattleRoyaleData.minimum_players ) return;
		
		//check round can be started
		bool CanFireNewRound = true;
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			//if any round is in progress but not started, we are waiting to start it & cannot launch a new round at this time
			if(round.inProgress && !round.RoundStarted)
			{
				CanFireNewRound = false;
				break;
			}
		}
		
		if(CanFireNewRound)
		{
			foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
			{
				if(!round.inProgress && !round.RoundStarted)
				{
					BRLOG("DAYZBR: ROUND START CALL");
					round.PlayerCountReached();
					return;
				}
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
		
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			round.RemovePlayer(player);
		}
		m_BattleRoyaleDebug.RemovePlayer(player);
		
	}
	
	override bool allowFallDamage(PlayerBase plr)
	{
		
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			if(round.ContainsPlayer(plr))
				return round.RoundStarted;
		}
		
		return true;
	}
	//player tick. process them for which class they are in
	override void OnPlayerTick(PlayerBase player, float ticktime)
	{
		if ( !GetGame().IsServer() ) return;

		if(m_BattleRoyaleDebug.ContainsPlayer(player))
		{
			m_BattleRoyaleDebug.OnPlayerTick(player,ticktime);
			return;
		}
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			if(round.ContainsPlayer(player))
			{
				round.OnPlayerTick(player,ticktime);
				return;
			}
		}
		//TODO: process clients that are bugged (they could also be between states)
	}

	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		if ( !GetGame().IsServer() ) return;

		// on player killed
		if(m_BattleRoyaleDebug.ContainsPlayer(killed))
		{
			m_BattleRoyaleDebug.OnPlayerKilled(killed,killer);
			return;
		}
		
		foreach(ref BattleRoyaleRound round : m_BattleRoyaleRounds)
		{
			if(round.ContainsPlayer(killed))
			{
				round.OnPlayerKilled(killed,killer);
				return;
			}
		}
		//TODO: client died but was bugged?
	}
}



