#ifdef SERVER
class BattleRoyaleLastRound: BattleRoyaleState
{
    ref BattleRoyaleState m_PreviousState;

    int i_RoundTimeInSeconds;
    bool b_DoZoneDamage;
    int i_DamageTickTime;
    float f_Damage;

    array<int> lock_notif_min;
    array<int> lock_notif_sec;

    bool b_IsZoneLocked;

    protected ref Timer m_FinalZoneLockTimer;
    protected ref array<ref Timer> m_MessageTimers;

    void BattleRoyaleLastRound(BattleRoyaleState previous_state)
    {
        m_PreviousState = previous_state;

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();
        BattleRoyaleZoneData m_ZoneSettings = m_Config.GetZoneData();

        lock_notif_min =  m_ZoneSettings.zone_notification_minutes;
        lock_notif_sec =  m_ZoneSettings.zone_notification_seconds;

        i_DamageTickTime = m_ZoneSettings.zone_damage_tick_seconds;
        f_Damage = m_ZoneSettings.zone_damage_delta;
        b_DoZoneDamage = m_ZoneSettings.enable_zone_damage;
        b_IsZoneLocked = false;

        m_MessageTimers = new array<ref Timer>;

        //--- static_timers[0] is the final circle's timer, which this state re-uses. It was read
        //--- unguarded, so an empty array indexed out of range in a constructor that runs inside
        //--- MissionServer.OnInit - i.e. it killed boot rather than degrading. Its sibling in
        //--- BattleRoyaleZone.GetZoneTimer() bounds-checks; this one did not.
        bool have_static_timer = false;
        if (m_ZoneSettings.shrink_type == 3)
            have_static_timer = (m_ZoneSettings.static_timers && m_ZoneSettings.static_timers.Count() > 0);

        if (have_static_timer)
        {
            i_RoundTimeInSeconds = m_ZoneSettings.static_timers[0];
        } else {
            if (m_ZoneSettings.shrink_type == 3)
                BattleRoyaleUtils.Warn("[BattleRoyaleLastRound] zone_settings.static_timers is empty - falling back to round_duration_minutes for the final round.");

            i_RoundTimeInSeconds = 60 * m_GameSettings.round_duration_minutes;
        }
    }

    override void Activate()
    {
        //--- Halfway, not the 80% a normal round uses - hence a SEPARATE constant from
        //--- BR_ZONE_LOCK_FRACTION rather than a shared one. The endgame has no travel to fund: the
        //--- circle is already where it is going to be, and this timer only governs how long the
        //--- survivors get before it closes on them.
        int time_till_lock = (i_RoundTimeInSeconds * 1000) * BR_ZONE_ENDGAME_LOCK_FRACTION;

        int i;
        int min;
        int sec;
        int val;
        //--- notification message timers
        for(i = 0; i < lock_notif_min.Count();i++)
        {
            min = lock_notif_min[i];
            val = time_till_lock - (min*60*1000);
            if(val > 0)
                m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeToEndMinutes", new Param1<int>( min ), false) ); //we need to store the object in case it's automatically deconstructed ?
        }
        for(i = 0; i < lock_notif_sec.Count();i++)
        {
            sec = lock_notif_sec[i];
            val = time_till_lock - (sec*1000);
            if(val > 0)
                m_MessageTimers.Insert( AddTimer(val / 1000.0, this, "NotifyTimeToEndSeconds", new Param1<int>( sec ), false) ); //we need to store the object in case it's automatically deconstructed ?
        }

        //lock zone event
        m_FinalZoneLockTimer = AddTimer( time_till_lock / 1000.0, this, "LockFinalZone", NULL, false);

        //timer before the final circle locks. Below the AddTimer above, not before it: SendCountdown
        //reads the remaining time off the timer itself, so the timer has to exist first.
        SendCountdown( m_FinalZoneLockTimer );

        //send play area to clients
        ref BattleRoyalePlayArea m_PreviousArea = NULL;
        if(GetPreviousZone())
            m_PreviousArea = GetPreviousZone().GetArea();

        //tell client the current play has not changed (note that if this is the first round, then the current area will be NULL )
        //--- The comment above acknowledges the NULL case and the code then dereferenced it anyway.
        //--- Guarded exactly as the sibling in 6_BattleRoyaleRound.Activate() already is; skipping
        //--- the send is correct, since "no current area" IS "no boundary yet" on the client.
        if(m_PreviousArea)
            SendCurrentPlayArea( m_PreviousArea.GetCenter(), m_PreviousArea.GetRadius() );

        //tell the client the future zone is NULL (no future zone)
        SendFuturePlayArea( "0 0 0", 0.0, false );

        super.Activate();
    }

    override void Deactivate()
    {
        if ( m_FinalZoneLockTimer && m_FinalZoneLockTimer.IsRunning() )
        {
            m_FinalZoneLockTimer.Stop();
        }

        for(int i = 0; i < m_MessageTimers.Count(); i++)
        {
            if ( m_MessageTimers[i] && m_MessageTimers[i].IsRunning() )
            {
                m_MessageTimers[i].Stop();
            }
        }

        //we just deactivated this round (players not yet transfered from previous state)
        super.Deactivate();
    }

    override bool SkipState(BattleRoyaleState _previousState)
    {
        //only one side (or less) remaining, must skip to win state
        //
        //--- The two tests this replaced were ANDed, so a party of two left standing satisfied
        //--- "players > 1" and refused to skip - the final round activated and deactivated inside a
        //--- single tick, which is what the doubled RemoveAllPlayers/AddPlayer pair in the log was.
        //--- Harmless in itself, but it is the same missing group test that stalled the state before
        //--- this one; both now ask BattleRoyaleState.IsOneSideLeft.
        return BattleRoyaleState.IsOneSideLeft( _previousState.GetPlayers() );
    }

	override string GetName()
	{
		return "Last Gameplay State";
	}

	//--- Needs its own override: this state extends BattleRoyaleState directly, not BattleRoyaleRound.
	override bool AllowsSpectate()
	{
		return true;
	}

	override bool IsComplete() //return true when this state is complete & ready to transfer to the next state
	{
		if(IsActive())
		{
			if(BattleRoyaleState.IsOneSideLeft( GetPlayers() ))
				Deactivate();
		}

		return super.IsComplete();
	}

	override void OnPlayerKilled(PlayerBase player, Object source)
	{
		super.OnPlayerKilled( player, source );
	}

    override void OnPlayerTick(PlayerBase player, float timeslice)
    {
        //determine if the player needs to take damage
        bool do_damage = b_IsZoneLocked;
        if(!do_damage)
        {
            //--- NULL when no round actually played before this one - there is no boundary to be
            //--- outside of yet, so nobody takes damage until the final zone locks.
            BattleRoyaleZone current_zone = GetPreviousZone();

            //--- A zero-radius area is the placeholder circle at the world origin left behind when
            //--- generation could not produce a real one; treating it as a boundary damages every
            //--- player on the map. Same guard as 6_BattleRoyaleRound.OnPlayerTick.
            bool zone_is_real = false;
            if(current_zone && current_zone.GetArea())
                zone_is_real = (current_zone.GetArea().GetRadius() > 0);

            if(b_DoZoneDamage && zone_is_real)
            {
                float radius = current_zone.GetArea().GetRadius();
                vector center = current_zone.GetArea().GetCenter();

                vector playerPos = player.GetPosition();

                //distance needs to be done in 2D, not 3D, set Z coord to 0 so this can be done
                playerPos[1] = 0;
                center[1] = 0;
                float distance = vector.Distance(playerPos, center);
                if(distance >= radius)
                {
                    do_damage = true;
                }
            }
        }

        if(do_damage)
        {
            if(player.time_until_damage <= 0)
            {
                //DAMAGE
                MessagePlayerUntranslated(player, "STR_BR_TAKING_DAMAGE");
                player.GetSymptomManager().QueueUpPrimarySymptom(SymptomIDs.SYMPTOM_PAIN_HEAVY);
                //--- auto_delete FALSE - see the identical call in 6_BattleRoyaleRound for why the
                //--- vanilla default (true) would delete the corpse, and why the fact that it
                //--- currently does not is an upstream bug rather than a guarantee.
                player.DecreaseHealthCoef( f_Damage, false );
                //--- Same reason as the kill feed hint below, but BR-owned: the death recap has to
                //--- name the zone on a server where Extra/KillFeed/ is not built at all.
                //--- Consumed by BattleRoyaleKillAttribution.ConsumeZoneHint.
                player.br_zone_damage_ms = GetGame().GetTime();
#ifdef KILLFEED
                //--- Scripted damage reaches EEKilled with the player as their own killer, so
                //--- without this the kill feed cannot tell a zone death from starvation.
                KillFeedAPI.NoteEnvironmentalDamage( player, KillFeedCause.ZONE );
#endif
                player.time_until_damage = i_DamageTickTime; //reset timer
            }
            player.time_until_damage -= timeslice;
        }
        else
        {
            player.time_until_damage = Math.Min(i_DamageTickTime, player.time_until_damage + timeslice);
        }

        super.OnPlayerTick(player, timeslice);
    }

    bool IsLocked()
    {
        return b_IsZoneLocked;
    }

    void LockFinalZone()
    {
        //TODO: this doesn't fucking work | if zone is null, no damage occurs
        SendCurrentPlayArea( "0 0 0", 0.0 );
        SendFuturePlayArea( "0 0 0", 0.0, false );
        b_IsZoneLocked = true;

        //--- Nothing left to count down to: the match now ends when one side is left standing,
        //--- which has no deadline. An explicit clear rather than letting the client tick to zero
        //--- on its own - the client no longer ticks anything.
        SendCountdown( NULL );
    }

    //--- The circle actually in play before the final round. A skipped round's zone was generated
    //--- but never activated and never sent to a client, so it must not be treated as a boundary.
    BattleRoyaleZone GetPreviousZone()
    {
        BattleRoyaleRound prev_round;
        if(Class.CastTo(prev_round, m_PreviousState))
        {
            if(prev_round.WasSkipped())
                return NULL;

            return prev_round.GetZone();
        }

        return NULL;
    }

    //TODO: both of these need added to battleroyaleconstants & use string replace to append minutes.ToString()
    void NotifyTimeToEndMinutes(int minutes)
    {
		MessagePlayersUntranslated("STR_BR_ZONE_WILL_DISAPPEAR_MINUTE", minutes.ToString());
    }

    void NotifyTimeToEndSeconds(int seconds)
    {
		MessagePlayersUntranslated("STR_BR_ZONE_WILL_DISAPPEAR_SECOND", seconds.ToString());
    }
}
