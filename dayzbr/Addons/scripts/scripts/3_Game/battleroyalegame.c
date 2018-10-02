modded class DayZGame
{
	override void OnRPC(PlayerIdentity sender, Object target, int rpc_type, ParamsReadContext ctx)
	{
		Event_OnRPC.Invoke( sender, target, rpc_type, ctx );
		
		//Print("["+ GetGame().GetTime().ToString() +"] => DayZGame::OnRPC = "+ rpc_type);
		if (target)
		{
			// call rpc on target
			target.OnRPC(sender, rpc_type, ctx);
		}
		else
		{
			// global rpc's handling
			if(rpc_type == MRPCs.RPC_BR_SEND_GLOBAL_MSG)
			{
				ref Param1<string> p_text = new Param1<string>("");
				ctx.Read( p_text );
				
				string display_text = p_text.param1;
				GetGame().Chat(  display_text, "colorImportant"  );
			}
			if(rpc_type == MRPCs.RPC_BR_SET_INPUT)
			{
				ref Param1<bool> p_input = new Param1<bool>(false);
				ctx.Read(p_input);
				Human human = Human.Cast(GetGame().GetPlayer());
				human.GetInputController().SetDisabled(p_input.param1);
			}
			if (rpc_type == MRPCs.RPC_CLIENT_ACTION_SPECTATOR)
			{
				//GetGame().SelectSpectator( sender, "DayZSpectator", GetPosition() );
			}
		}
	}
}