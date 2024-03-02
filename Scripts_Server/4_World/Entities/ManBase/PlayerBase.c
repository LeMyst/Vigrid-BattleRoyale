modded class PlayerBase
{
    //credit to wardog for the quick fix for client localplayers grabbing
    private static autoptr array<PlayerBase> s_LocalPlayers = new array<PlayerBase>();

#ifndef SERVER
    void PlayerBase()
    {
        if (s_LocalPlayers)
        {
            s_LocalPlayers.Insert(this);
        }
    }

    void ~PlayerBase()
    {
        if (s_LocalPlayers)
        {
            int localIndex = s_LocalPlayers.Find(this);
            if (localIndex >= 0)
            {
                s_LocalPlayers.Remove(localIndex);
            }
        }
    }
#endif

    static void GetLocalPlayers(out array<PlayerBase> players)
    {
#ifndef SERVER
        players = new array<PlayerBase>();
        players.Copy(s_LocalPlayers);
#endif
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
        Print(" Call To Disable (" + disabled + ") Player Input ");
        HumanInputController controller = GetInputController();
        controller.SetDisabled( disabled );
        /*controller.OverrideMovementSpeed( disabled, 0 );
        controller.OverrideMovementAngle( disabled, 0 );
        controller.OverrideMeleeEvade( disabled, false );
        controller.OverrideRaise( disabled, false );
        controller.OverrideMovementAngle( disabled, 0 );*/
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

    //Temp fix for disabling character saving
    override bool Save()
    {
        return false;
    }

    void Heal()
    {
#ifdef SERVER
        //TODO: stop player from bleeding!
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

        // unknown effect, don't alter yet
        // player.GetStatStomachEnergy().Set(100);
        // player.GetStatDiet().Set(100);

        // think max stamina does not break the game
        GetStatStamina().Set(GetStatStamina().GetMax());

        // required for repairing and stuff, so no need to change for godmode
        //player.GetStatSpecialty().Set(100);
#endif
    }

    override void ResetPlayer(bool set_max)
    {
#ifdef SERVER
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
#endif
    }
}
