#ifdef SERVER
class BattleRoyaleStartMatch: BattleRoyaleState
{
    protected int i_TimeToUnlock;
    protected bool b_IsGameplay;
    protected int i_FirstRoundDelay;
    protected bool b_ShowFirstZone;
    protected bool b_ArtillerySound;

    protected ref array<PlayerBase> m_PlayerList;

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
        #ifdef DIAG
        i_TimeToUnlock = 1;
        #else
        i_TimeToUnlock = m_GameSettings.time_until_teleport_unlock;
        #endif

        b_ShowFirstZone = m_GameSettings.show_first_zone_at_start;

        b_ArtillerySound = m_GameSettings.artillery_sound;

        b_IsGameplay = false;

        m_PlayerList = new array<PlayerBase>;

        m_MessageTimers = new array<ref Timer>;
    }

    override string GetName()
    {
        return DAYZBR_SM_START_MATCH_NAME;
    }

    override void Activate()
    {
        super.Activate();

        //send start match RPC (this will enable UI such as kill count)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "StartMatch", new Param1<bool>(true), true); //don't need a param, but id rather keep it just so i know nothing wierd occurs (eventually find out if we can remove it)

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
        if(GetPlayers().Count() <= 1 && IsActive())
        {
            BattleRoyaleUtils.Trace(GetName() + " IsComplete!");
            // TODO: clean call queue?
            // TODO: toggle to debug game
            Deactivate();
        }

        return super.IsComplete();
    }

    //TODO: add this to battleroyaleconstants and use string replace to insert seconds_till
    void MessageUnlock(int seconds_till)
    {
        string second = "second";
        if(seconds_till != 1)
            second = "seconds";

        MessagePlayers("Starting in " + seconds_till.ToString() + " " + second + "!");
    }

    void UnlockPlayers()
    {
        m_PlayerList.InsertAll( m_Players );

        //enable player input on clients (we'll do this on server in another thread)
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(false), true);

        GetGame().GameScript.Call(this, "HandleUnlock", NULL); //spin up unlocking thread
    }

    void ShowFirstZone()
    {
        // Show first circle
        BattleRoyaleUtils.Trace("[BattleRoyaleStartMatch] Show first circle");
        BattleRoyaleZone m_Zone = new BattleRoyaleZone;
        m_Zone = m_Zone.GetZone(BattleRoyalePrepare.i_StartingZone);
        m_Zone.OnActivate( GetPlayers() ); //hand players over to the zone (for complex zone size/position calculation)
        ref BattleRoyalePlayArea m_ThisArea = m_Zone.GetArea();

        BattleRoyaleUtils.Trace(m_ThisArea.GetCenter());
        BattleRoyaleUtils.Trace(m_ThisArea.GetRadius());

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "UpdateFuturePlayArea", new Param3<vector, float, bool>( m_ThisArea.GetCenter(), m_ThisArea.GetRadius(), false ), true);
    }

    void HandleUnlock()
    {
        BattleRoyaleUtils.Trace("HandleUnlock");
        for(int i = 0; i < m_PlayerList.Count(); i++)
        {
            PlayerBase player = m_PlayerList[i];
            BattleRoyaleUtils.Trace("Unlock " + player.GetIdentity().GetName());

            player.DisableInput(false); //This will re-enable input
			//DayZPlayerSyncJunctures.SendPlayerUnconsciousness( player , false );
        }

        MessagePlayersUntranslated( "STR_BR_MATCH_STARTED" );
        MessagePlayersUntranslatedTimed( "STR_BR_UNSTUCK_INFORMATION", 90 );

        b_IsGameplay = true;
    }

    void StartZoning()
    {
        Deactivate();
    }

    void DeferredUnstuck( PlayerBase player )
	{
		if( !player.wait_unstuck )
		{
			player.wait_unstuck = true;
			MessagePlayerUntranslated( player, "STR_BR_UNSTUCK_TELEPORTATION" );
			BattleRoyaleUtils.Trace( player.GetIdentity().GetName() + " asked for an unstuck teleportation." );
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLaterByName(this, "Unstuck", Math.RandomFloat(1, 3) * 1000 , false, new Param1<PlayerBase>( player ));
		}
		else
		{
			MessagePlayerUntranslated( player, "STR_BR_ALREADY_REQUESTED_UNSTUCK" );
		}
	}

	void Unstuck( PlayerBase player )
	{
		for(int search_pos = 1; search_pos <= 50; search_pos++)
		{
			float radius, angle, x, z, y;
			vector player_position, playerDir, direction;

			radius = 10.0;
			angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
			player_position = player.GetPosition();
			x = player_position[0] + ( radius * Math.Cos(angle) );
			z = player_position[2] + ( radius * Math.Sin(angle) );
			y = GetGame().SurfaceY(x, z);

			if( IsSafeForTeleport(x, y, z, false) )
			{
				playerDir = vector.YawToVector( Math.RandomFloat(0, 360) );
				direction = Vector(playerDir[0], 0, playerDir[1]);

				ScriptJunctureData pCtx = new ScriptJunctureData;
				pCtx.Write( Vector(x, y, z) );
				pCtx.Write( direction );
				player.SendSyncJuncture( 88, pCtx );
				player.SetSynchDirty();
				player.wait_unstuck = false;
				break;
			}
		}
	}

    override void OnPlayerKilled(PlayerBase player, Object killer)
    {
        if(!b_IsGameplay)
        {
            Error("Player killed before gameplay!");
            return;
        }

        super.OnPlayerKilled( player, source );
    }
}
