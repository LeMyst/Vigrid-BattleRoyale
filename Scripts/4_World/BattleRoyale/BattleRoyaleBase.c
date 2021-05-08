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
	
	ref MatchData GetMatchData()
	{
		return NULL;
	}
	
	bool IsDebug() 
	{
		return true;
	}

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
		PlayerBase targetBase = PlayerBase.Cast( target );
		if(!targetBase) return;
		
		EntityAI item = null;
		switch(data.param1) {
			case 0:
				HumanInventory inv = targetBase.GetHumanInventory();
				if(inv)
				{
					item = inv.FindAttachment(InventorySlots.BODY);
					Print("[Base] Setting Shirt Skin");
				}
				break;
			case 1:
				item = targetBase.GetItemInHands();
				Print("[Base] Setting Gun Skin");
				break;
		}
		if(item)
		{
			Print(item.GetType());

			//apply texture data to item
			int i;
			ref array<string> values = data.param2;
			Print("Texture Count: " + values.Count().ToString());
			for(i = 0; i < values.Count(); i++)
			{
				string texture = values.Get(i);
				Print("Setting texture: " + i.ToString() + "," + texture);
				item.SetObjectTexture(i, texture);
			}
			values = data.param3;
			Print("Material Count: " + values.Count().ToString());
			for(i = 0; i < values.Count(); i++)
			{
				string material = values.Get(i);
				Print("Setting material: " + i.ToString() + "," + material);
				item.SetObjectMaterial(i, material);
			}
		}

		if(type == CallType.Server)
		{
			Print("Rebroadcasting!");
			//rebroadcast to all clients
			ref array<string> textures = data.param2; //this should solve an issue where the value is derefed and not sent to the client...
			ref array<string> materials = data.param3; 
			Param3<int, ref array<string>, ref array<string>> new_data = new Param3<int, ref array<string>, ref array<string>>(data.param1, textures, materials);
			Print(new_data);
			GetRPCManager().SendRPC( RPC_DAYZBRBASE_NAMESPACE, "SetItemSkin", new_data, false , NULL, targetBase);
		}
	}
}