class BattleRoyaleBase
{
	void BattleRoyaleBase()
	{
		GetRPCManager().AddRPC( RPC_DAYZBRBASE_NAMESPACE, "SetItemSkin", this);
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
			item.SetObjectTexture(0, data.param1);
		}
		if(type == CallType.Server)
		{
			//request was recieved by server - rebroadcast to all clients
			ref Param1<string> gun_value = new Param1<string>( data.param1 );
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetGunTexture", gun_value, false, NULL, targetBase);
		}
	}
	void SetItemSkin(CallType type, ref ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param3< int, ref array<string>, ref array<string> > data;
		
		//read & check sanity
		if( !ctx.Read( data ) ) return;
		if(!target) return;
		PlayerBase targetBase = PlayerBase.Cast(target);
		if(!targetBase) return;
		
		EntityAI item = null;
		switch(data.param1) {
			case 0:
				HumanInventory inv = targetBase.GetHumanInventory();
				if(inv)
				{
					item = inv.FindAttachment(InventorySlots.BODY);
				}
				break;
			case 1:
				item = targetBase.GetItemInHands();
				break;
		}
		if(item)
		{
			Print(item.GetType());

			//apply texture data to item
			int i;
			ref array<string> values = data.param2;
			for(i = 0; i < values.Count(); i++)
			{
				item.SetObjectTexture(i, values.Get(i));
			}
			values = data.param3;
			for(i = 0; i < values.Count(); i++)
			{
				item.SetObjectMaterial(i, values.Get(i));
			}
		}

		if(type == CallType.Server)
		{
			//rebroadcast to all clients
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetItemSkin", data, false , NULL, targetBase);
		}
	}
}