#ifdef SERVER
class BattleRoyaleStartMatch: BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_IsGameplay;
    protected int i_FirstRoundDelay;
    protected bool b_ShowFirstZone;
    protected bool b_ArtillerySound;

    protected ref array<PlayerBase> a_PlayerList;

    protected ref array<ref Timer> m_MessageTimers;
    protected ref Timer m_UnlockTimer;
    protected ref Timer m_ShowFirstZone;
    protected ref Timer m_ZoneStartTimer;

    void BattleRoyaleStartMatch()
    {
        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleGameData m_GameSettings = m_Config.GetGameData();

        i_FirstRoundDelay = (60 * m_GameSettings.round_duration_minutes) / 2;

        //seconds until unlock
#ifdef DIAG_DEVELOPER
        i_TimeToUnlock = 1;
#else
        i_TimeToUnlock = m_GameSettings.time_until_teleport_unlock;
#endif

        b_ShowFirstZone = m_GameSettings.show_first_zone_at_start;

        b_ArtillerySound = m_GameSettings.artillery_sound;

        b_IsGameplay = false;

        a_PlayerList = new array<PlayerBase>;

        m_MessageTimers = new array<ref Timer>;
    }

    override string GetName()
    {
        return "Start Match State";
    }

    //--- Only once the freeze has lifted. Dying during the frozen warmup keeps the old behaviour,
    //--- because there is nothing to watch yet and the match has not really begun.
    override bool AllowsSpectate()
    {
        return b_IsGameplay;
    }

    override void Activate()
    {
        super.Activate();

        //--- Back to open proximity voice for the match. Deliberately before HandleUnlock() runs its
        //--- per-player DisableInput(false), so the later call can only ever loosen the policy.
        BattleRoyaleVoice.ClearAll();

        //send start match RPC (this will enable UI such as kill count)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", new Param1<bool>(true), true); //don't need a param, but id rather keep it just so i know nothing wierd occurs (eventually find out if we can remove it)

        //--- Hot zones. Spawn selection normally sets this reference first and this is a harmless
        //--- re-assert, but spawn selection is OPTIONAL (enable_spawn_selection_menu) - without this
        //--- call a server with the menu turned off would never show a hot zone at all. Every step
        //--- is on its own line: an array read nested inside a call inside another call is the shape
        //--- that has thrown "NULL pointer to instance" here before.
        BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
        if (br_instance)
        {
            int starting_zone = GetDynamicStartingZone(i_NumStartingPlayers);
            ref BattleRoyaleZone start_zone = BattleRoyaleZone.GetZone(starting_zone);
            if (start_zone)
            {
                ref BattleRoyalePlayArea start_area = start_zone.GetArea();
                if (start_area)
                    br_instance.SetHotZoneReference(start_area.GetCenter(), start_area.GetRadius());
            }
        }

#ifdef KILLFEED
        //--- Kills count from here on, so the feed goes live with the match.
        KillFeedAPI.SetActive( true );
#endif

        int max_time = i_TimeToUnlock - 1;
        for(int i = max_time; i > 0; i--)
        {
            m_MessageTimers.Insert( AddTimer(i, this, "MessageUnlock", new Param1<int>(i_TimeToUnlock - i), false) );
        }

        m_UnlockTimer = AddTimer(i_TimeToUnlock, this, "UnlockPlayers", NULL, false);

        if (b_ShowFirstZone)
            m_ShowFirstZone = AddTimer(i_TimeToUnlock, this, "ShowFirstZone", NULL, false);

        m_ZoneStartTimer = AddTimer( i_FirstRoundDelay, this, "StartZoning", NULL, false);

        //timer before first zone appears
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetCountdownSeconds", new Param1<int>(i_FirstRoundDelay), true);
    }

    override void Deactivate()
    {
        //deactivate all one-time timers
        if ( m_UnlockTimer && m_UnlockTimer.IsRunning() )
            m_UnlockTimer.Stop();

        if ( m_ZoneStartTimer && m_ZoneStartTimer.IsRunning() )
            m_ZoneStartTimer.Stop();

        for(int i = 0; i < m_MessageTimers.Count(); i++)
        {
            if ( m_MessageTimers[i] && m_MessageTimers[i].IsRunning() )
                m_MessageTimers[i].Stop();
        }

        super.Deactivate();
    }

    override bool IsComplete()
    {
        //--- Groups, not players. This state runs for the whole first-zone countdown -
        //--- round_duration_minutes / 2, 90 s at the shipped defaults - which is long enough for a
        //--- party to clear the field, and the raw player test this used to carry could never see
        //--- that: the winning party still holds two or more live players. The match then sat here
        //--- until StartZoning fired on its own timer, and the win screen only appeared at the end of
        //--- the countdown. See BattleRoyaleState.IsOneSideLeft.
        if(IsActive() && BattleRoyaleState.IsOneSideLeft( GetPlayers() ))
        {
            BattleRoyaleUtils.Trace(GetName() + " IsComplete!");
            // TODO: clean call queue?
            // TODO: toggle to debug game
            Deactivate();
        }

        return super.IsComplete();
    }

	void MessageUnlock(int seconds_till)
	{
		if(seconds_till > 1)
			MessagePlayersUntranslated("STR_BR_STARTING_IN_SECONDS", seconds_till.ToString());
		else
			MessagePlayersUntranslated("STR_BR_STARTING_IN_SECOND", seconds_till.ToString());
	}

    void UnlockPlayers()
    {
        a_PlayerList.InsertAll( m_Players );

        //enable player input on clients (we'll do this on server in another thread)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(false), true);

        GetGame().GameScript.Call(this, "HandleUnlock", NULL); //spin up unlocking thread
    }

    void ShowFirstZone()
    {
        // Show first circle
        BattleRoyaleUtils.Trace("[BattleRoyaleStartMatch] Show first circle");
        BattleRoyaleZone m_Zone = new BattleRoyaleZone;
        //--- i_NumStartingPlayers, not the live count: the rounds decide which zone to skip to from
        //--- the countdown snapshot, so using the live count here can advertise a different circle.
        m_Zone = m_Zone.GetZone(GetDynamicStartingZone(i_NumStartingPlayers));
        m_Zone.OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
        ref BattleRoyalePlayArea m_ThisArea = m_Zone.GetArea();

        BattleRoyaleUtils.Trace(m_ThisArea.GetCenter());
        BattleRoyaleUtils.Trace(m_ThisArea.GetRadius());

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), false ), true);
    }

    void HandleUnlock()
    {
        BattleRoyaleUtils.Trace("HandleUnlock");
        for(int i = 0; i < a_PlayerList.Count(); i++)
        {
            PlayerBase player = a_PlayerList[i];
            BattleRoyaleUtils.Trace("Unlock " + player.GetIdentity().GetName());

            player.DisableInput(false); //This will re-enable input
			//DayZPlayerSyncJunctures.SendPlayerUnconsciousness( player , false );
        }

        MessagePlayersUntranslated( "STR_BR_MATCH_STARTED" );
        MessagePlayersUntranslatedTimed( "STR_BR_UNSTUCK_INFORMATION", i_FirstRoundDelay );

        b_IsGameplay = true;

#ifdef VIGRID_SAFEZONE
        //--- Lift the lobby truce here rather than in Activate(): input is still locked through the
        //--- warm-up countdown, so this is the first instant a player could actually shoot back.
        VigridSafeZoneAPI.SetActive( false );
#endif

        OpenLeaderboardMatch();
    }

    /**
     *  Snapshot the field for the leaderboard, at the exact instant deaths begin to count.
     *
     *  Taken here rather than at lobby lock on purpose: a lobby that fills to 16 groups and then
     *  loses 14 of them during the countdown is a 2-group match, and must not pay out like a
     *  16-group one.
     *
     *  Both the field size and each player's own group size are resolved here, in 5_Mission, and
     *  handed down as plain data - VigridPartyAPI lives in Party's 4_World and the leaderboard code
     *  sits in this mod's 4_World, which has no declared dependency on that PBO.
     */
    void OpenLeaderboardMatch()
    {
        int field_size = GetPlayers().Count();
        ref map<string, int> group_sizes = new map<string, int>();

        //--- The match summary needs two more snapshots taken at this same instant: which squad each
        //--- player belongs to, and what everyone was called while they all still had an identity.
        ref map<string, int> group_index = new map<string, int>();
        ref map<string, string> names = new map<string, string>();
        bool grouped = false;

        //--- Seeded with the SOLO partition first, then overwritten by the party block below. That
        //--- ordering is what lets this method have one code path and no #else branch: with the addon
        //--- absent, or present but disabled, every player simply keeps their own index.
        for (int s = 0; s < GetPlayers().Count(); s++)
        {
            PlayerBase solo = GetPlayers().Get(s);
            if (!solo)
                continue;
            if (solo.player_steamid == "")
                continue;

            group_index.Set(solo.player_steamid, s);
            names.Set(solo.player_steamid, BattleRoyaleKillAttribution.NameOfPlayer(solo));
        }

#ifdef VIGRID_PARTY
        array<ref array<PlayerBase>> groups = VigridPartyAPI.GetGroups( GetPlayers() );
        field_size = groups.Count();

        //--- IsReady(), never the #ifdef. Party ships in this repo so the guard is true on every
        //--- server, while party_settings.json can still switch the manager off - in which case
        //--- GetGroups degrades to one group per player and field_size becomes numerically identical
        //--- to the player count. That is correct for partitioning and wrong for any label saying
        //--- "squads", which is the mechanism behind issue #158.
        grouped = VigridPartyAPI.IsReady();

        for (int g = 0; g < groups.Count(); g++)
        {
            array<PlayerBase> party_group = groups.Get(g);
            for (int p = 0; p < party_group.Count(); p++)
            {
                PlayerBase member = party_group.Get(p);
                if (!member)
                    continue;
                if (member.player_steamid == "")
                    continue;

                group_sizes.Set(member.player_steamid, party_group.Count());
                group_index.Set(member.player_steamid, g);
            }
        }
#endif

        BattleRoyaleLeaderboard.GetInstance().BeginMatch( field_size, group_sizes );
        BattleRoyaleMatchStats.GetInstance().BeginMatch( field_size, grouped, group_index, names );
    }

    void StartZoning()
    {
        //--- Deferred: this is a timer callback, i.e. inside TimerQueue.Tick. See
        //--- BattleRoyaleState.DeactivateDeferred().
        DeactivateDeferred();
    }

    //--- The warm-up is the state this feature was written for: players have just been dropped into
    //--- the world and input is locked until HandleUnlock, so a bad landing is not recoverable by
    //--- moving. The implementation itself now lives on BattleRoyaleState, shared with the lobby.
    override bool AllowsUnstuck()
    {
        return true;
    }

    override void OnPlayerKilled(PlayerBase player, Object source)
    {
        if(!b_IsGameplay)
        {
            BattleRoyaleUtils.Info("Player killed before gameplay!");
            return;
        }

        super.OnPlayerKilled( player, source );
    }
}
