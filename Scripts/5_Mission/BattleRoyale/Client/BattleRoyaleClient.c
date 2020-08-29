#define BR_BETA_LOGGING

class BattleRoyaleClient extends BattleRoyaleBase
{

	protected ref BattleRoyalePlayArea m_CurrentPlayArea;
	protected ref BattleRoyalePlayArea m_FuturePlayArea;
	

	#ifdef BR_BETA_LOGGING
	bool print_once_tick = true;
	#endif
	void BattleRoyaleClient()
	{
		Init();
	}
	void Init()
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleClient::Init()");
		#endif

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetPlayerCount", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetFade", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetInput", this );

		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateCurrentPlayArea", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", this );
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
		//TODO: destroy Fade UI
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
			Print("BattleRoyale: Network Update Current Play Area");

			Print(data.param1);
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
			Print("BattleRoyale: Network Update Future Play Area");

			Print(data.param1);
			m_FuturePlayArea = data.param1;
		}
	}




	protected string last_surface_type = "";

	override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		string surface_type;
		vector pos = player.GetPosition();
		GetGame().SurfaceGetType(pos[0], pos[2], surface_type);

		if(surface_type != last_surface_type)
		{
			Print(surface_type);
		}
		last_surface_type = surface_type;
		
		#ifdef BR_BETA_LOGGING
		if(print_once_tick)
		{
			print_once_tick = false;
			BRPrint("BattleRoyaleClient::OnPlayerTick() => Running!");
		}
		#endif
		
	}
	override void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		#ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyaleClient::OnPlayerKilled()");
		#endif
		
	}
}