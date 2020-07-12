modded class PlayerBase
{
	float time_until_heal = 0;
	float time_until_damage = 0;

	bool allow_fade = false;

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

	void Heal()
	{
		//NOTE: this heal function was done by legodev, not sure it's performance, we'll have to see

		SetHealth("", "Health", GetMaxHealth("", "Health"));
		SetHealth("", "Blood", GetMaxHealth("", "Blood"));
		SetHealth("", "Shock", GetMaxHealth("", "Shock"));
		
		// GetStatStomachVolume + GetStatStomachWater > 1000 == STUFFED!
		GetStatStomachVolume().Set(250);
		GetStatStomachWater().Set(250);
		
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