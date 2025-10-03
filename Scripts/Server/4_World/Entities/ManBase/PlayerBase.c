#ifdef SERVER
modded class PlayerBase
{
    float time_until_heal = 0;
    float time_until_damage = 0;
    float time_until_move = 0;

    bool allow_fade = false;

    float time_between_net_sync = 1000;
    float time_since_last_net_sync = 0;
    bool force_result = true;

    bool wait_unstuck = false;

    string owner_id = "";

	int br_position = -1;
	string player_steamid = "";

	vector spawn_pos = vector.Zero;

	PlayerBase last_unconscious_source;
	float m_UnconsciousStartTime;

#ifdef SPECTATOR
    bool UpdateHealthStatsServer(float hp, float blood, float delta)
    {
        time_since_last_net_sync += delta;
        if(time_since_last_net_sync > time_between_net_sync)
        {
            time_since_last_net_sync = 0;
            bool return_me; //we'll need to flip force_result back to false, so we need a temp variable to store this value in
            if ( force_result || UpdateHealthStats(hp, blood) )
                return_me = true;
            force_result = false;
            return return_me;
        }
        else
        {
            //if we ever get a "true" in this update system, we need to store it for the next sync tick
            if ( UpdateHealthStats(hp, blood) )
                force_result = true;
            
            return false;
        }
    }
#endif

	void SetBRPosition( int position )
	{
		br_position = position;
	}

	int GetBRPosition()
	{
		return br_position;
	}

    override void OnScheduledTick(float deltaTime)
    {
        super.OnScheduledTick(deltaTime);

        BattleRoyaleBase m_BR = GetBR();
        if(m_BR)
            m_BR.OnPlayerTick(this, deltaTime);
    }

    override void EEKilled( Object killer )
    {
        super.EEKilled( killer );

        BattleRoyaleBase m_BR = GetBR();
        if(m_BR)
            m_BR.OnPlayerKilled(this, killer);
    }

    override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		PlayerBase playerSource;

		if ( source.IsPlayer() )  // Fists
			playerSource = PlayerBase.Cast( source );
		else
			playerSource = PlayerBase.Cast( source.GetHierarchyParent() );

		// Test if the source is a player
		if ( playerSource )
		{
			last_unconscious_source = playerSource;
		} else {
			last_unconscious_source = NULL;
			BattleRoyaleUtils.Info("Player " + GetIdentity().GetName() + " was hit by an unknown source.");
		}
	}

	override void OnUnconsciousStop(int pCurrentCommandID)
	{
		super.OnUnconsciousStop(pCurrentCommandID);

		if (last_unconscious_source)
		{
			last_unconscious_source = NULL;
		}
	}

	override void OnSyncJuncture( int pJunctureID, ParamsReadContext pCtx )
	{
		super.OnSyncJuncture( pJunctureID, pCtx );

		switch( pJunctureID )
		{
			case 88:
				vector position, direction;
				pCtx.Read( position );
				pCtx.Read( direction );
				if ( position )
				{
					SetDynamicPhysicsLifeTime( 0.001 );
					dBodyActive( this, ActiveState.INACTIVE );
					SetPosition( position );
					dBodyActive( this, ActiveState.ACTIVE );
				}
				if ( direction )
				{
					SetDirection( direction );
				}
				break;
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

        //SetBleedingBits(0);
        if ( m_BleedingManagerServer )
            m_BleedingManagerServer.RemoveAllSources();

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
        //GetStatHeatBuffer().Set(25); //give players a + by default

        // seems unused
        // player.GetStatHeatIsolation().Set(100);

        // we don't want shaking -> limit is 0.008
        GetStatTremor().Set(GetStatTremor().GetMin());

        // wet if > 0.2
        GetStatWet().Set(0);

        // unknown effect, don't alter yet
        // player.GetStatStomachEnergy().Set(100);
        // player.GetStatDiet().Set(100);

        // think max stamina does not break the game
        GetStatStamina().Set(GetStatStamina().GetMax());

        // required for repairing and stuff, so no need to change for godmode
        //player.GetStatSpecialty().Set(100);
    }

    override void ResetPlayer(bool set_max)
    {
        //clear stomach content
        GetStomach().ClearContents();

        // bleeding sources
        if ( m_BleedingManagerServer )
            m_BleedingManagerServer.RemoveAllSources();

        // Modifiers
        bool hasAreaExposureModifier, hasMaksModifier;
        if (GetModifiersManager())
        {
            hasAreaExposureModifier = GetModifiersManager().IsModifierActive(eModifiers.MDF_AREAEXPOSURE);
            hasMaksModifier = GetModifiersManager().IsModifierActive(eModifiers.MDF_MASK);
            GetModifiersManager().DeactivateAllModifiers();

            if (hasAreaExposureModifier)
                GetModifiersManager().ActivateModifier(eModifiers.MDF_AREAEXPOSURE);

            if (hasMaksModifier)
                GetModifiersManager().ActivateModifier(eModifiers.MDF_MASK);
        }

        // Stats
        if (GetPlayerStats())
        {
            int bloodType = GetStatBloodType().Get();
            GetPlayerStats().ResetAllStats();
            GetStatBloodType().Set(bloodType);
        }

        if (m_StaminaHandler)
            m_StaminaHandler.SetStamina(GameConstants.STAMINA_MAX);

        // Agents
        if (m_AgentPool)
            m_AgentPool.RemoveAllAgents();

        // Damage System
        DamageZoneMap zones = new DamageZoneMap();
        DamageSystem.GetDamageZoneMap(this, zones);
        SetHealth("", "Health", GetMaxHealth("","Health"));
        SetHealth("", "Shock", GetMaxHealth("","Shock"));
        SetHealth("", "Blood", GetMaxHealth("","Blood"));

        for (int i = 0; i < zones.Count(); i++)
        {
            string zone = zones.GetKey(i);
            SetHealth(zone, "Health", GetMaxHealth(zone,"Health"));
            SetHealth(zone, "Shock", GetMaxHealth(zone,"Shock"));
            SetHealth(zone, "Blood", GetMaxHealth(zone,"Blood"));
        }

        // uncon
        if (IsUnconscious())
            DayZPlayerSyncJunctures.SendPlayerUnconsciousness(this, false);

        // set max
        if (set_max)
        {
            GetStatWater().Set(GetStatWater().GetMax());
            GetStatEnergy().Set(GetStatEnergy().GetMax());
        }

        // fix up inventory
        FixAllInventoryItems();

        //remove bloody hands
        PluginLifespan moduleLifespan = PluginLifespan.Cast(GetPlugin(PluginLifespan));
        moduleLifespan.UpdateBloodyHandsVisibilityEx(this, eBloodyHandsTypes.CLEAN);
    }

    void SetSpawnPos(vector pos)
	{
		BattleRoyaleUtils.Trace("SetSpawnPos: " + pos);
		spawn_pos = pos;
	}

	vector GetSpawnPos()
	{
		return spawn_pos;
	}
};
