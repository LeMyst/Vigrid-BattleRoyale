class BattleRoyaleBase
{
	void BattleRoyaleBase()
	{
		GetRPCManager().AddRPC( RPC_DAYZBRBASE_NAMESPACE, "SetShirtTexture", this);
	}

	void OnPlayerTick(PlayerBase player, float timeslice)
	{}
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{}
	void Update(float delta)
	{}
	

	void SetShirtTexture(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param2< string, string > data;
		if( !ctx.Read( data ) ) return; //i have absolutely no idea if i have to read this twice?
		
		if(!target) return;
			
		PlayerBase targetBase = PlayerBase.Cast(target);

		if(!targetBase) return;

		HumanInventory inv = targetBase.GetHumanInventory();
		if(inv)
		{
			//set shirt texture
			EntityAI shirt = inv.FindAttachment(InventorySlots.BODY);
			if(shirt) 
			{
				Print(shirt.GetType());
				shirt.SetObjectTexture(0,data.param1);
				shirt.SetObjectTexture(1,data.param2);
				shirt.SetObjectTexture(2,data.param2); 
			}
		}

		if(type == CallType.Server)
		{
			ref Param2<string, string> value_string = new Param2<string, string>(data.param1, data.param2);
			
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetShirtTexture", value_string, false , NULL, targetBase);
		}


	}
}