modded class PlayerBaseClient
{	
	bool FORCE_FADE = false;

	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);
		
		
		
		switch(rpc_type)
		{
			
			//SERVER-SIDED RPC RECEIVER 
			
			case MRPCs.RPC_CLIENT_INCREASE_STATS:
			
				ref Param1<float> p_synch = new Param1<float>(0);
				ctx.Read( p_synch );
				
				GetStatWater().Add(p_synch.param1);
				GetStatEnergy().Add(p_synch.param1);
				GetStatStomachSolid().Add(p_synch.param1);
				break;
				
				
				
			//CLIENT-SIDED RPC RECEIVER	
			case MRPCs.RPC_BR_SEND_CLIENT_MSG:
				ref Param1<string> p_text = new Param1<string>("");
				ctx.Read( p_text );
				
				string display_text = p_text.param1;
				this.MessageImportant(display_text);
				break;
			
			case MRPCs.RPC_BR_SET_INPUT:
				ref Param1<bool> p_input = new Param1<bool>(true);
				ctx.Read(p_input);
				GetInputController().SetDisabled(p_input.param1);
				break;
				
			case MRPCs.RPC_BR_FADE_IN:
				FORCE_FADE = true;
				string text = "DayZ Battle Royale";
				GetGame().GetUIManager().ScreenFadeIn(0, text, FadeColors.BLACK, FadeColors.WHITE); //FadeColors.DARK_RED
				break;
				
			case MRPCs.RPC_BR_FADE_OUT:
				FORCE_FADE = false;
				GetGame().GetUIManager().ScreenFadeOut(0);
				break;
				
			case MRPCs.RPC_CLIENT_ACTION_SPECTATOR:
				GetGame().SelectSpectator( sender, "DayZSpectator", GetPosition() );
				break;
		}
	}
}