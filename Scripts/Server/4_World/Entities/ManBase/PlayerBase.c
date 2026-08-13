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
    //--- Tick time before which this player may not request another unstuck. wait_unstuck alone only
    //--- covers the 1-3 s while one is in flight; this is what stops F2 being held down as
    //--- fast-travel now that the lobby answers it too. See BattleRoyaleState.DeferredUnstuck.
    float next_unstuck_time = 0;

    string owner_id = "";

	int br_position = -1;
	string player_steamid = "";
	//--- Cached alongside player_steamid because PlayerIdentity is already NULL by the time the
	//--- leaderboard records a disconnect, and the ladder has to render a name for someone who is
	//--- no longer on the server.
	string player_name = "";

	/**
	 *  Overwrite vanilla's own cached name (playerbase.c:221, protected and server-only, seeded from
	 *  the identity in EEInit). Doing this is what lets Party and KillFeed show a resolved name
	 *  through the public GetCachedName() without either addon naming a BattleRoyale symbol.
	 */
	void BR_SetCachedName(string name)
	{
		m_CachedPlayerName = name;
	}

	//--- Server-side kill tally for the current match. Kills were previously only ever pushed to the
	//--- owning client over the AddPlayerKill RPC, so nothing on the server could score them.
	int br_kills = 0;

	vector spawn_pos = vector.Zero;

	//--- GetGame().GetTime() of the last play-area damage tick, dropped by 6_BattleRoyaleRound and
	//--- 7_BattleRoyaleLastRound. Scripted damage reaches EEKilled with the player as their own
	//--- source, so this is the only way the death recap can tell the zone from starvation or a fall.
	//--- Consumed (read then zeroed) by BattleRoyaleKillAttribution.ConsumeZoneHint, so a stale hint
	//--- cannot mislabel the next environmental death.
	int br_zone_damage_ms = 0;

	//--- Health and blood as they were immediately BEFORE the hit currently being processed, latched
	//--- in EEOnDamageCalculated so EEHitBy can measure what the hit actually removed. br_prehit_ms
	//--- is the guard: the latch is only trusted within the same frame it was taken.
	float br_prehit_health = 0;
	float br_prehit_blood = 0;
	int br_prehit_ms = -1;

	PlayerBase last_unconscious_source;
	//--- SteamID64 of whoever is responsible for the hit that downed this player, which is NOT always
	//--- expressible as an object: an explosive's owner may already be dead or disconnected. Set
	//--- alongside last_unconscious_source and used when that reference is NULL.
	string last_unconscious_source_uid = "";
	float m_UnconsciousStartTime;

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

		//--- Guarded: not every hit carries a source, and both dereferences below were unguarded.
		if ( !source )
		{
			last_unconscious_source = NULL;
			last_unconscious_source_uid = "";
			return;
		}

		PlayerBase playerSource = BattleRoyaleKillAttribution.ResolvePlayerSource( source );

		//--- The uid is the one that survives an explosive. A free-standing grenade has NO hierarchy
		//--- parent, so playerSource is NULL for it and the object reference could never credit
		//--- anybody - which is why being downed by a grenade and then disconnecting scored nothing.
		//--- ResolveKillerUid reads the activator the device recorded when it was armed.
		last_unconscious_source = playerSource;
		last_unconscious_source_uid = BattleRoyaleKillAttribution.ResolveKillerUid( this, source );

		if ( !playerSource && last_unconscious_source_uid == "" )
			BattleRoyaleUtils.Trace("Player " + GetCachedName() + " was hit by an unknown source.");

		BR_NoteDamageDealt();
	}

	/**
	 *  Latch this player's health and blood BEFORE the hit lands.
	 *
	 *  TotalDamageResult reports COMPUTED damage, not APPLIED damage, so a 400-damage headshot on a
	 *  player with 12 HP left reports 400. EEHitBy runs after application and so cannot recover the
	 *  pre-hit value. This is the only hook that runs first.
	 *
	 *  SUPER FIRST, and return on false. Extra/SafeZone declares its own modded PlayerBase with this
	 *  same override in a different PBO, and neither addon requires the other, so their relative
	 *  chain order is not pinned by anything. This shape makes the order irrelevant: whichever way
	 *  round they end up, a hit SafeZone cancels never reaches EEHitBy, so lobby punches can never be
	 *  scored - and if we latch first and SafeZone then cancels, the latch is simply never consumed.
	 */
	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if ( !super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef) )
			return false;

		br_prehit_health = GetHealth("", "Health");
		br_prehit_blood = GetHealth("", "Blood");
		br_prehit_ms = GetGame().GetTime();

		return true;
	}

	/**
	 *  Credit whoever just hit this player, measured as the health/blood the hit actually removed.
	 *
	 *  Measured as a DELTA rather than read off TotalDamageResult for three reasons: it clamps for
	 *  free, because both pools floor at zero, so a headshot on a dying player scores what they had
	 *  left instead of a four-figure overkill number; it accounts for armour; and it picks up the
	 *  shock-to-health transfer vanilla applies inside EEHitBy itself, which the reported damage does
	 *  not include. The existing override calls super first, so GetHealth here is fully post-hit.
	 *
	 *  BOTH pools, because DayZ death is Health <= 0 OR Blood <= 0 and a firearm body shot is mostly
	 *  a blood hit - roughly 100 blood against a 5000 pool versus 30 health against 100. Health alone
	 *  would under-report every gunfight and report ZERO for a bleed-out kill. Math.Max rather than a
	 *  sum so a hit doing both is not counted twice, and normalising by this player's own maxima
	 *  keeps it honest if another mod resizes the pools. The result reads as "one full-health player
	 *  == 100 damage".
	 */
	protected void BR_NoteDamageDealt()
	{
		BattleRoyaleMatchStats stats = BattleRoyaleMatchStats.GetInstance();
		if ( !stats.IsRecording() )
			return;

		//--- Reuse the uid EEHitBy already resolved - do not call ResolveKillerUid a second time. It
		//--- is "" for self-damage, the zone, falls, drowning, infected, animals and your own
		//--- grenade, so this single test excludes every non-player cause.
		if ( last_unconscious_source_uid == "" )
			return;

		string victim_uid = player_steamid;
		if ( victim_uid == "" && GetIdentity() )
			victim_uid = GetIdentity().GetPlainId();

		//--- Crediting a player for shooting their own squadmate reads as a bug on a summary card.
		if ( stats.AreTeammates(last_unconscious_source_uid, victim_uid) )
			return;

		//--- A STAMP, not a bool. Some scripted damage paths never route through
		//--- EEOnDamageCalculated, and a latch left over from an earlier hit would clamp this one
		//--- against a health value that is seconds old.
		if ( br_prehit_ms != GetGame().GetTime() )
			return;

		/**
		 *  Were they ALIVE BEFORE this hit? That is the question, and it is not the same as IsAlive().
		 *
		 *  This used to be a plain `if ( !IsAlive() ) return;` to stop corpse-shooting from scoring.
		 *  It also threw away the KILLING BLOW, because vanilla's IsAlive() is !IsDamageDestroyed() -
		 *  purely health-based - and EEHitBy runs AFTER the engine has applied the damage. A hit that
		 *  takes health to zero therefore reports IsAlive() false by the time we look, so the one hit
		 *  that decided the fight was the one hit never credited.
		 *
		 *  Measured 2026-08-13: a knife kill on a full-health player scored 76 instead of >= 100,
		 *  which is what surfaced this - the per-hit deltas cannot sum to less than the total health
		 *  actually lost unless hits are being dropped.
		 *
		 *  The pre-hit latch answers it exactly: a corpse was already at zero before the hit and is
		 *  still refused, while the fatal blow was above zero and now counts. The delta clamps itself
		 *  to whatever they had left, so the killing hit scores what it actually took and no overkill.
		 */
		if ( br_prehit_health <= 0 )
			return;

		float max_health = GetMaxHealth("", "Health");
		float max_blood = GetMaxHealth("", "Blood");
		if ( max_health <= 0 )
			return;
		if ( max_blood <= 0 )
			return;

		float health_lost = br_prehit_health - GetHealth("", "Health");
		float blood_lost = br_prehit_blood - GetHealth("", "Blood");

		float dealt = Math.Max( health_lost / max_health, blood_lost / max_blood );
		dealt = dealt * 100;

		if ( dealt <= 0 )
			return;

		stats.NoteDamage( last_unconscious_source_uid, victim_uid, dealt );
	}

	override void OnUnconsciousStop(int pCurrentCommandID)
	{
		super.OnUnconsciousStop(pCurrentCommandID);

		last_unconscious_source = NULL;
		last_unconscious_source_uid = "";
	}

	override void OnSyncJuncture( int pJunctureID, ParamsReadContext pCtx )
	{
		super.OnSyncJuncture( pJunctureID, pCtx );

		switch( pJunctureID )
		{
			case BR_SYNC_JUNCTURE_TELEPORT:
				vector position, direction;
				pCtx.Read( position );
				pCtx.Read( direction );
				if ( position )
				{
					//--- Seated just above the resolved ground rather than exactly on it, so the
					//--- capsule never starts inside the surface. This is a few centimetres and
					//--- nothing depends on it; it is emphatically NOT a drop the engine is expected
					//--- to turn into a fall - see BR_TELEPORT_DROP_HEIGHT for the metre that tried
					//--- that and left everybody hovering.
					position[1] = position[1] + BR_TELEPORT_DROP_HEIGHT;

					SetDynamicPhysicsLifeTime( 0.001 );
					dBodyActive( this, ActiveState.INACTIVE );
					SetPosition( position );
					dBodyActive( this, ActiveState.ACTIVE );
				}
				if ( direction )
				{
					SetDirection( direction );
				}

				//--- Repositioning alone leaves the running movement command running, so a player
				//--- teleported off a ladder or out of an unfinished fall arrives still playing that
				//--- animation - which, for the unstuck teleport, is the exact state it exists to
				//--- escape. It has to happen HERE rather than before the teleport: clearing the
				//--- command while the player is still standing on the ladder just lets the engine
				//--- re-enter it on the next tick, which is what BattleRoyalePrepare saw.
				//---
				//--- Requested rather than done outright: this is a juncture handler, not the
				//--- command tick. See BR_NotifyTeleported.
				BR_NotifyTeleported();
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
