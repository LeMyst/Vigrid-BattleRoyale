modded class PlayerBase
{
	//credit to wardog for the quick fix for client localplayers grabbing
	private static autoptr array<PlayerBase> s_LocalPlayers = new array<PlayerBase>();
    void PlayerBase()
    {
        if (IsMissionClient() && s_LocalPlayers)
        {
            s_LocalPlayers.Insert(this);
        }
    }

    void ~PlayerBase()
    {
        if (IsMissionClient() && s_LocalPlayers)
        {
            int localIndex = s_LocalPlayers.Find(this);
            if (localIndex >= 0)
            {
                s_LocalPlayers.Remove(localIndex);
            }
        }
    }
	static void GetLocalPlayers(out array<PlayerBase> players)
    {
        if (IsMissionClient())
        {
            players = new array<PlayerBase>();
            players.Copy(s_LocalPlayers);
        }
    }

	float time_until_heal = 0;
	float time_until_damage = 0;
	float time_until_move = 0;

	bool allow_fade = false;

	float health_percent = -1;
	float blood_percent = -1;

	float time_between_net_sync = 1000;
	float time_since_last_net_sync = 0;
	bool force_result = true;

	string owner_id = "";

	void DisableInput(bool disabled)
	{
		Print(" Call To Disable Player Input ");
		HumanInputController controller = GetInputController();
		controller.OverrideMovementSpeed( disabled, 0 );
		controller.OverrideMeleeEvade( disabled, false );
		controller.OverrideRaise( disabled, false );
		controller.OverrideMovementAngle( disabled, 0 );
	}

	bool UpdateHealthStatsServer(float hp, float blood, float delta)
	{
		time_since_last_net_sync += delta;
		if(time_since_last_net_sync > time_between_net_sync)
		{
			time_since_last_net_sync = 0;
			bool result = UpdateHealthStats(hp, blood);
			bool return_me = force_result || result; //we'll need to flip force_result back to false, so we need a temp variable to store this value in
			force_result = false;
			return return_me;
		}
		else
		{
			force_result = force_result || UpdateHealthStats(hp, blood); //if we ever get a "true" in this update system, we need to store it for the next sync tick
			return false;
		}
	}

	bool UpdateHealthStats(float hp, float blood)
	{
		//Print("Updating player health stats");
		bool changed = (health_percent != hp) || (blood_percent != blood);
		health_percent = hp;
		blood_percent = hp;
		return changed;
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
	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if(GetGame().IsServer() && source && GetBR())
		{
			//! server - hit event!
			Man killer = source.GetHierarchyRootPlayer();
			if(killer && killer.IsPlayer())
			{
				BattleRoyaleDebug m_Debug;
				BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
				if(!Class.CastTo(m_Debug, m_CurrentState))
				{
	
					PlayerBase shooter = PlayerBase.Cast( killer );
					if(shooter && shooter.GetIdentity() && this.GetIdentity())
					{
						string shooterid = shooter.GetIdentity().GetPlainId();
						BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().Hit( this.GetIdentity().GetPlainId(), shooterid, this.GetPosition(), GetGame().GetTime() );
					}
				}
			}
		}
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}
	
	//Temp fix for disabling character saving
	override bool Save()
	{
		return false;
	}
	void Heal()
	{
		//TODO: sstop player from beleding!
		//NOTE: this heal function was done by legodev, not sure it's performance, we'll have to see

		SetHealth("", "Health", GetMaxHealth("", "Health"));
		SetHealth("", "Blood", GetMaxHealth("", "Blood"));
		SetHealth("", "Shock", GetMaxHealth("", "Shock"));
		
		// GetStatStomachVolume + GetStatStomachWater > 1000 == STUFFED!

		SetBleedingBits(0);
		
		//--- legacy function (need to access m_PlayerStomach [PlayerStomach] and try from there)
		//GetStatStomachVolume().Set(250);
		//GetStatStomachWater().Set(250);
		
		// for bone regen: water = 2500 and energy = 4000 so 5000 should be ok
		GetStatWater().Set(4500);
		GetStatEnergy().Set(4500);
		// is get max an good idea?
		// player.GetStatWater().Set(player.GetStatWater().GetMax());
		// player.GetStatEnergy().Set(player.GetStatEnergy().GetMax());
		
		
		// default body temperature is  37.4 -> HYPOTHERMIC_TEMPERATURE_TRESHOLD = 35.8
		//player.GetStatTemperature().Set(37.4);
		
		// BURNING_TRESHOLD = 199 -> 100 should be fine
		//GetStatHeatComfort().Set(100); //no temperature flashing
		GetStatHeatBuffer().Set(25); //give players a + by default
		
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