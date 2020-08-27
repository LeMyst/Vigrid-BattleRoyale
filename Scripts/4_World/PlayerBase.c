modded class PlayerBase
{
	float time_until_heal = 0;
	float time_until_damage = 0;

	bool allow_fade = false;

	void PlayerBase()
	{	
		//only register RPCs on client (this is a server->client function not reverse)
		if(!IsMissionHost())
		{
			GetRPCManager().AddRPC( RPC_DAYZBR_NAMESPACE, "RPC_DisableInput", this );
		}
	}

	void DisableInput(bool disabled)
	{
		HumanInputController controller = GetInputController();
		controller.OverrideMovementSpeed( disabled, 0 );
		controller.OverrideMeleeEvade( disabled, false );
		controller.OverrideRaise( disabled, false );
		controller.OverrideMovementAngle( disabled, 0 );

		if(IsMissionHost())
		{
			GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "RPC_DisableInput", new Param1<bool>( disabled ), true, GetIdentity(), this); //disable user input
		}
	}
	void RPC_DisableInput(CallType type, ParamsReadContext ctx, ref PlayerIdentity sender, ref Object target)
	{
		Param1<bool> data;
		if( !ctx.Read( data ) ) 
		{
			Error("FAILED TO READ RPC_DisableInput RPC");
			return;
		}
		if ( type == CallType.Client )
		{
			DisableInput( data.param1 );
		}
	}

	override void OnScheduledTick(float deltaTime)
	{
		super.OnScheduledTick(deltaTime);
		
		BattleRoyaleBase m_BR = GetBR();
		if(m_BR)
		{
			m_BR.OnPlayerTick(this, deltaTime);
		}
	}
	override void EEKilled( Object killer )
	{
		super.EEKilled( killer );
		
		BattleRoyaleBase m_BR = GetBR();
		if(m_BR)
		{
			m_BR.OnPlayerKilled(this, killer);
		}
	}
	
	void RemoveAllItems_Safe()
	{
		array<EntityAI> itemsArray = new array<EntityAI>;
		ItemBase item;
		GetInventory().EnumerateInventory(InventoryTraversalType.PREORDER, itemsArray);
		
		for (int i = 0; i < itemsArray.Count(); i++)
		{
			Class.CastTo(item, itemsArray.Get(i));
			if (item && !item.IsInherited(SurvivorBase))	
			{
				//this extra check prevents RPT spam 
				InventoryLocation src = new InventoryLocation;
				if(item.GetInventory().GetCurrentInventoryLocation(src))
				{
					GetInventory().LocalDestroyEntity(item);
				}
			}
		}
	}

	//Temp fix for disabling character saving
	override bool Save()
	{
		return false;
	}
	void Heal()
	{
		//NOTE: this heal function was done by legodev, not sure it's performance, we'll have to see

		SetHealth("", "Health", GetMaxHealth("", "Health"));
		SetHealth("", "Blood", GetMaxHealth("", "Blood"));
		SetHealth("", "Shock", GetMaxHealth("", "Shock"));
		
		// GetStatStomachVolume + GetStatStomachWater > 1000 == STUFFED!

		
		
		//--- legacy function (need to access m_PlayerStomach [PlayerStomach] and try from there)
		//GetStatStomachVolume().Set(250);
		//GetStatStomachWater().Set(250);
		
		// for bone regen: water = 2500 and energy = 4000 so 5000 should be ok
		GetStatWater().Set(5000);
		GetStatEnergy().Set(5000);
		// is get max an good idea?
		// player.GetStatWater().Set(player.GetStatWater().GetMax());
		// player.GetStatEnergy().Set(player.GetStatEnergy().GetMax());
		
		
		// default body temperature is  37.4 -> HYPOTHERMIC_TEMPERATURE_TRESHOLD = 35.8
		//player.GetStatTemperature().Set(37.4);
		
		// BURNING_TRESHOLD = 199 -> 100 should be fine
		GetStatHeatComfort().Set(100);
		
		// seems unused
		// player.GetStatHeatIsolation().Set(100);
		
		// we don't want shaking -> limit is 0.008
		GetStatTremor().Set(GetStatTremor().GetMin());
		
		// wet if > 0.2
		GetStatWet().Set(0);
		
		// unknow effect, don't alter yet
		// player.GetStatStomachEnergy().Set(100);
		// player.GetStatDiet().Set(100);
		
		// think max stamima does not break the game
		GetStatStamina().Set(GetStatStamina().GetMax());
		
		// required for repairing and stuff, so no need to change for godmode
		//player.GetStatSpecialty().Set(100);

	}
}