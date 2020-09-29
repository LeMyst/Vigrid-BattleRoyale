class BattleRoyaleBase
{
	void BattleRoyaleBase()
	{
		GetRPCManager().AddRPC( RPC_DAYZBRBASE_NAMESPACE, "SetShirtTexture", this);
		GetRPCManager().AddRPC( RPC_DAYZBRBASE_NAMESPACE, "SetGunTexture", this);
	}

	void OnPlayerTick(PlayerBase player, float timeslice)
	{}
	void OnPlayerKilled(PlayerBase killed, Object killer)
	{}
	void Update(float delta)
	{}
	
	void SetGunTexture(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1< string > data;
		if( !ctx.Read( data ) ) return;
		if(!target) return;
		PlayerBase targetBase = PlayerBase.Cast(target);
		if(!targetBase) return;
		EntityAI item = targetBase.GetItemInHands();
		if(item)
		{
			Print("Applying gun texture!");
			Print(item.GetType());
			Print(data.param1);
			//weapon texture isn't index 0 anymore?
			item.SetObjectTexture(0, data.param1);
			item.SetObjectTexture(1, data.param1);
			item.SetObjectTexture(2, data.param1);
		}
		if(type == CallType.Server)
		{
			//request was recieved by server - rebroadcast to all clients
			ref Param1<string> gun_value = new Param1<string>( data.param1 );
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetGunTexture", gun_value, false, NULL, targetBase);
		}
	}
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