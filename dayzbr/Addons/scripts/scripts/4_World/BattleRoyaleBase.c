static const string RPC_DAYZBR_NAMESPACE = "DM-DayZBR";

class BattleRoyaleBase
{
	void BattleRoyaleBase()
	{
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SendGlobalMessage", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SendClientMessage", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "RequestAutowalk", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "IncreaseStats", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "RequestSuicide", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "SetInput", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ScreenFadeIn", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "ScreenFadeOut", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "EnterSpectator", this );
		GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "LeaveSpectator", this );
	}

	bool allowFallDamage(PlayerBase plr)
	{
		return true;
	}	
	
	void SendGlobalMessage( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data;
		if( !ctx.Read( data ) ) return;
        
		if( GetGame() )
		{
			GetGame().Chat( data.param1, "colorImportant" );
		}
	}

	void SendClientMessage( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< string > data;
		if( !ctx.Read( data ) ) return;
        
		if ( type == CallType.Client )
		{
			PlayerBase player = PlayerBase.Cast( target );

			if ( !player ) return;

			player.MessageImportant( data.param1 );
		}
	}

	void RequestAutowalk( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< bool > data;
		if( !ctx.Read( data ) ) return;
        
		if ( type == CallType.Server )
		{
			DayZPlayerImplement player = DayZPlayerImplement.Cast( target );

			if ( !player ) return;

			if( data.param1 )
			{
				player.GetInputController().OverrideMovementSpeed( true, 2 );
				player.GetInputController().OverrideMovementAngle( true, 1 );
			}
			else
			{
				player.GetInputController().OverrideMovementSpeed( false, 0 );
				player.GetInputController().OverrideMovementAngle( false, 0 );
			}
		}
	}

	void IncreaseStats( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< float > data;
		if( !ctx.Read( data ) ) return;
        
		if ( type == CallType.Server )
		{
			PlayerBase player = PlayerBase.Cast( target );

			if ( !player ) return;

			player.GetStatWater().Add( data.param1 );
			player.GetStatEnergy().Add( data.param1 );
			player.GetStatStomachSolid().Add( data.param1 );
		}
	}

	void RequestSuicide( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( type == CallType.Server )
		{
			PlayerBase player = PlayerBase.Cast( target );

			if ( !player ) return;

			player.SetHealth( "", "", 0.0 );
		}
	}

	void SetInput( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		Param1< bool > data;
		if( !ctx.Read( data ) ) return;

		if ( type == CallType.Client )
		{
			PlayerBase player = PlayerBase.Cast( target );

			if ( !player ) return;

			// player.GetInputController().SetDisabled( data.param1 );
			player.GetInputController().SetDisabled( false );

			player.GetInputController().OverrideMovementSpeed( data.param1, 0 );
			player.GetInputController().OverrideMeleeEvade( data.param1, false );
			player.GetInputController().OverrideRaise( data.param1, false );
			player.GetInputController().OverrideMovementAngle( data.param1, 0 );
			//player.GetInputController().OverrideAimChangeY( data.param1, 0 ); //Don't use OverrideAimChangeY as it causes Aiming bug?
		}
	}

	void AllowLookInput( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( type == CallType.Client )
		{
			PlayerBase player = PlayerBase.Cast( target );

			if ( !player ) return;

			player.GetInputController().OverrideAimChangeY( false, 0 );
		}
	}

	void ScreenFadeIn( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( type == CallType.Client )
		{
			PlayerBase player = GetGame().GetPlayer();
			player.FORCE_FADE = true;

			string text = "DayZ Battle Royale";
			GetGame().GetUIManager().ScreenFadeIn( 0, text, FadeColors.BLACK, FadeColors.WHITE ); //FadeColors.DARK_RED
		}
	}

	void ScreenFadeOut( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
		if ( type == CallType.Client )
		{
			PlayerBase player = GetGame().GetPlayer();
			player.FORCE_FADE = false;
			GetGame().GetUIManager().ScreenFadeOut( 0.5 );
		}
	}

	// TODO: Implement
	void EnterSpectator( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
	}

	// TODO: Implement
	void LeaveSpectator( CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target )
	{
	}

	void OnPlayerKilled(PlayerBase killed, Object killer)
	{
		Print("ERROR: RUNNING BASE ON KILLED");
	}

	void OnPlayerTick(PlayerBase player, float ticktime)
	{
		Print("ERROR: RUNNING BASE ON TICK");
	}
}