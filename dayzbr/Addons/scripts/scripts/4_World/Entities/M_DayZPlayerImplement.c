modded class DayZPlayerImplement
{
	//Our modded player RPC events (server or client)
	override void OnRPC(PlayerIdentity sender, int rpc_type, ParamsReadContext ctx)
	{
		super.OnRPC(sender, rpc_type, ctx);

		
		switch(rpc_type)
		{
			
			//--- Server RPC Events (run from client->Server)
			case MRPCs.RPC_CLIENT_REQUEST_AUTOWALK:
				Param1<bool> autowalkParam = new Param1<bool>(false);

				ctx.Read(autowalkParam);

				if(autowalkParam.param1)
				{
					GetInputController().OverrideMovementSpeed( true, 2 );
					GetInputController().OverrideMovementAngle( true, 1 );
				}
				else
				{
					GetInputController().OverrideMovementSpeed( false, 0 );
					GetInputController().OverrideMovementAngle( false, 0 );
				}
				break;
		
			case MRPCs.RPC_CLIENT_INCREASE_STATS:
			
				ref Param1<float> p_synch = new Param1<float>(0);
				ctx.Read( p_synch );
				
				PlayerBase.Cast(this).GetStatWater().Add(p_synch.param1);
				PlayerBase.Cast(this).GetStatEnergy().Add(p_synch.param1);
				PlayerBase.Cast(this).GetStatStomachSolid().Add(p_synch.param1);
				break;
				
			case MRPCs.RPC_CLIENT_REQUEST_SUICIDE:
				PlayerBase.Cast(this).SetHealth("", "", 0.0);
				break;
		}
	}
	
}