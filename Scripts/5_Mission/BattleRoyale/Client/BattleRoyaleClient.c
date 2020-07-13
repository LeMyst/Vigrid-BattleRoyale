#define BR_BETA_LOGGING

class BattleRoyaleClient extends BattleRoyaleBase
{

	ref BattleRoyalePlayArea m_CurrentPlayArea;
	ref BattleRoyalePlayArea m_FuturePlayArea;

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
	



	void FadeIn()
	{
		PlayerBase player = GetGame().GetPlayer();
		player.allow_fade = true;
		GetGame().GetUIManager().ScreenFadeIn( 0, BATTLERYALE_FADE_MESSAGE, FadeColors.BLACK, FadeColors.WHITE ); //FadeColors.DARK_RED
	}
	void FadeOut()
	{
		PlayerBase player = GetGame().GetPlayer();
		player.allow_fade = false;
		//GetGame().GetUIManager().ScreenFadeOut( 0.5 ); //unnecessary as it is automatically done by missiongameplay::update
	}
	void DisableUserInput()
	{
		PlayerBase player = GetGame().GetPlayer();

		if ( !player ) 
		{
			Error("CANNOT DISABLE INPUT! INVALID USER");
			return;
		}

		player.GetInputController().SetDisabled( false );
		player.GetInputController().OverrideMovementSpeed( true, 0 );
		player.GetInputController().OverrideMeleeEvade( true, false );
		player.GetInputController().OverrideRaise( true, false );
		player.GetInputController().OverrideMovementAngle( true, 0 );
	}
	void EnableUserInput()
	{
		PlayerBase player = GetGame().GetPlayer();

		if ( !player ) 
		{
			Error("CANNOT ENABLE INPUT! INVALID USER");
			return;
		}

		player.GetInputController().SetDisabled( false );
		player.GetInputController().OverrideMovementSpeed( false, 0 );
		player.GetInputController().OverrideMeleeEvade( false, false );
		player.GetInputController().OverrideRaise( false, false );
		player.GetInputController().OverrideMovementAngle( false, 0 );
	}
	


	
	//Client RPC calls
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
			if( data.param1 )
				DisableUserInput();
			else
				EnableUserInput();
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