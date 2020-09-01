class BattleRoyaleClient extends BattleRoyaleBase
{

	protected ref BattleRoyalePlayArea m_CurrentPlayArea;
	protected ref BattleRoyalePlayArea m_FuturePlayArea;
	protected ref ScriptCallQueue m_CallQueue;
	
	protected int i_Kills; //TODO: this needs to be done differently (most likely)
	protected bool b_MatchStarted;
	protected int i_SecondsRemaining;

	void BattleRoyaleClient()
	{
		b_MatchStarted = false;
		i_Kills = 0;
		i_SecondsRemaining = 0;
		m_CallQueue = new ScriptCallQueue;

		Init();
	}
	void Init()
	{
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetFade", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetInput", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "AddPlayerKill", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", this );

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", this );

		
		m_CallQueue.CallLater(this.OnSecond, 1000, true); //call onsecond every second
	}

	//--- note: these return NULL of there is no area referenced for next or current area
	BattleRoyalePlayArea GetPlayArea()
	{
		return m_CurrentPlayArea
	}
	BattleRoyalePlayArea GetNextArea()
	{
		return m_FuturePlayArea
	}
	
	override void Update(float delta)
	{
		MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );

		if(m_FuturePlayArea)
		{
			float distance = GetZoneDistance( m_FuturePlayArea );
			gameplay.UpdateZoneDistance( distance ); //update HUD element
		}

		m_CallQueue.Tick( delta );
	}


	protected float GetZoneDistance(BattleRoyalePlayArea play_area)
	{
		vector center = play_area.GetCenter();
		PlayerBase player = GetGame().GetPlayer();
		vector playerpos = player.GetPosition();

		//2d distance check
		center[1] = 0;
		playerpos[1] = 0; 
		float distance_from_center = vector.Distance(center, playerpos);
		float distance_from_outside = distance_from_center - play_area.GetRadius();
		return distance_from_outside;
	}

	protected void PlayerCountChanged(int new_count)
	{
		MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
		gameplay.UpdatePlayerCount( new_count );
	}
	protected void FadeIn()
	{
		PlayerBase player = GetGame().GetPlayer();
		MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
		Print("BattleRoyale: FADE IN!");
		//TODO: create Fade UI
	}
	protected void FadeOut()
	{
		PlayerBase player = GetGame().GetPlayer();
		MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
		Print("BattleRoyale: FADE OUT!");
	}
	protected void OnSecond()
	{
		MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
		if(i_SecondsRemaining > 0)
		{
			i_SecondsRemaining--;
			gameplay.UpdateCountdownTimer(i_SecondsRemaining);
		}
		else
		{
			gameplay.HideCountdownTimer();
		}
		
	}
	protected void AddPlayerKilled(int increase)
	{
		i_Kills += increase;
		MissionGameplay gameplay = MissionGameplay.Cast( GetGame().GetMission() );
		gameplay.UpdateKillCount(i_Kills);
	}
	protected void OnMatchStarted()
	{
		if(b_MatchStarted)
		{
			Error("Match started already but recieved another RPC?");
		}
		b_MatchStarted = true;
	}

	void SetShirt(string texture)
	{
		PlayerBase player = GetGame().GetPlayer();
		HumanInventory inv = player.GetHumanInventory();

		EntityAI shirt = inv.FindAttachment(InventorySlots.BODY);
		if(shirt)
		{
			ref Param1<string> shirt_value = new Param1<string>( texture );
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetShirtTexture", shirt_value, false , NULL, player);
		}
	}


	
	//Client RPC calls
	void StartMatch(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if ( type == CallType.Client )
		{
			OnMatchStarted();
		}
	}
	void AddPlayerKill(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		if ( type == CallType.Client )
		{
			AddPlayerKilled(1); //TODO: maybe we'll eventually store kills on the server & just send that across. Idk we'll figure it out
		}
	}
	void SetCountdownSeconds(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ SetCountdownSeconds RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			i_SecondsRemaining =  data.param1;
		}
	}



	void SetPlayerCount(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<int> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ SETPLAYERCOUNT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			PlayerCountChanged( data.param1 );
		}
	}
	void SetInput(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ SETINPUT RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			PlayerBase player = GetGame().GetPlayer();
			player.DisableInput( data.param1 );
		}
	}
	void SetFade(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ SETFADE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			if( data.param1 )
				FadeIn();
			else
				FadeOut();
		}
	}





	void UpdateCurrentPlayArea(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<ref BattleRoyalePlayArea> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ SETFADE RPC");
			return;
		}

		if ( type == CallType.Client )
		{
			m_CurrentPlayArea = data.param1;
		}
	}
	void UpdateFuturePlayArea(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<ref BattleRoyalePlayArea> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ SETFADE RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			m_FuturePlayArea = data.param1;
		}
	}



	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		//unused
	}
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		//unused	
	}
}