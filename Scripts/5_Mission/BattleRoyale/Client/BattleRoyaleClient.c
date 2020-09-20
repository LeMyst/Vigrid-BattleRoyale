class BattleRoyaleClient extends BattleRoyaleBase
{

	protected ref BattleRoyalePlayArea m_CurrentPlayArea;
	protected ref BattleRoyalePlayArea m_FuturePlayArea;
	protected ref ScriptCallQueue m_CallQueue;
	
	protected int i_Kills; //TODO: this needs to be done differently (most likely)
	protected bool b_MatchStarted;
	protected int i_SecondsRemaining;

	protected bool b_IsReady;

	void BattleRoyaleClient()
	{
		b_IsReady = false;
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
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ActivateSpectatorCamera", this );

		
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

		float distance;
		if(m_FuturePlayArea)
		{
			distance = GetZoneDistance( m_FuturePlayArea );
			gameplay.UpdateZoneDistance( distance ); //update HUD element
		}
		else
		{
			if(m_CurrentPlayArea)
			{
				distance = GetZoneDistance( m_CurrentPlayArea );
				gameplay.UpdateZoneDistance( distance );
			}
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

	void SetShirt(string ground_texture, string shirt_texture)
	{
		PlayerBase player = GetGame().GetPlayer();
		HumanInventory inv = player.GetHumanInventory();

		EntityAI shirt = inv.FindAttachment(InventorySlots.BODY);
		if(shirt)
		{
			ref Param2<string, string> shirt_value = new Param2<string, string>( ground_texture, shirt_texture );
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetShirtTexture", shirt_value, false , NULL, player);
		}
	}
	void ReadyUp()
	{
		if(b_IsReady)
			return; //already ready!
		
		b_IsReady = true; //this only runs once

		PlayerBase player = GetGame().GetPlayer();
		ref Param1<bool> ready_state = new Param1<bool>( true );  //perhaps this can be made togglable?
		GetRPCManager().SendRPC( RPC_DAYZBRSERVER_NAMESPACE, "PlayerReadyUp", ready_state, false , NULL, player);

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


	void ActivateSpectatorCamera(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Print("Activating Spectator Camera");
		BattleRoyaleCamera br_Camera;
		if ( Class.CastTo( br_Camera, Camera.GetCurrentCamera() ) )
		{
			br_Camera.SetActive( true );
			
			if ( GetGame().GetPlayer() )
			{
				GetGame().GetPlayer().GetInputController().SetDisabled( true );
			}

			//Gameplay changes
			MissionGameplay mission = MissionGameplay.Cast( GetGame().GetMission() );
			if ( mission )
			{
				Print("Initializing Spectator in Mission")
				//Enable spectator HUD elements
				mission.InitSpectator();
			}

		}
		else
		{
			Error("Failed to cast camera to BattleRoyaleCamera");
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