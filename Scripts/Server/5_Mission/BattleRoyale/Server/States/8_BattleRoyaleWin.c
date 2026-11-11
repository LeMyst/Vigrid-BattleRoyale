#ifdef SERVER
class BattleRoyaleWin: BattleRoyaleState
{
    int i_SecondsTillKick;
    bool complete;

    protected ref Timer m_KickTimer;

    void BattleRoyaleWin()
    {
        i_SecondsTillKick = 15; //TODO: config this
        complete = false;
    }

    //TODO: state functionality for winners!
    //TODO: Add party support
	override void Activate()
	{
		super.Activate();

#ifdef KILLFEED
		//--- The match is over; anything that dies from here on is not part of the story.
		KillFeedAPI.SetActive( false );
#endif

		//--- Tell every spectator the match is done. Idempotent - 9_BattleRoyaleRestart calls it too,
		//--- in case this state is ever skipped. Spectators are not in m_Players, so this cannot
		//--- affect the GetPlayers().Count() == 0 condition below or the winner kick.
		BattleRoyaleSpectators.GetInstance().EndAll();

		//--- The match is over, so there is nothing left to count down to. Without this the winner
		//--- watches the previous round's clock keep running under the win screen: the client holds
		//--- a deadline, and only a push clears it.
		SendCountdown( NULL );

		if(GetPlayers().Count() > 0)
		{
			string squad_names = BuildWinnerNames();

			for ( int k = 0; k < GetPlayers().Count(); k++ )
			{
				PlayerBase winner = GetPlayers()[k];
				if(!winner)
					continue;

				PlayerIdentity identity = winner.GetIdentity();
				if(identity)
				{
					BattleRoyaleUtils.Trace("[Win State] Winner!");
					BattleRoyaleUtils.Trace(identity.GetName());
					BattleRoyaleUtils.Trace(identity.GetFullName());
					BattleRoyaleUtils.Trace(identity.GetId());
					BattleRoyaleUtils.Trace(identity.GetPlainId());
				}
				HandleWinner(winner, squad_names);
			}
		}

		//--- Opportunistic: get the match on disk before the 15s kick window, in case the process
		//--- dies during it. The guaranteed flush is in 9_BattleRoyaleRestart. Flush() is a no-op
		//--- when nothing is pending, so the several call sites need no coordination.
		BattleRoyaleLeaderboard.GetInstance().Flush();

		m_KickTimer = AddTimer(i_SecondsTillKick, this, "KickWinner", NULL, false);
	}

    override string GetName()
    {
        return "Win State";
    }

    //--- KickWinner is the winners' only RecordExit; skipping it records the runner-up as winner.
    override bool AllowsAdminSkip()
    {
        return false;
    }

    override bool IsMatchOver()
    {
        return true;
    }

    override void Deactivate()
    {
        if ( m_KickTimer && m_KickTimer.IsRunning() )
            m_KickTimer.Stop();
        
        super.Deactivate();
    }

    override bool IsComplete()
    {
        if(GetPlayers().Count() == 0 && IsActive())
            Deactivate();
        
        return super.IsComplete(); //go to restart state when player disconnects
    }

    //! Every surviving winner's name, comma-separated, for the win screen's subtitle.
    protected string BuildWinnerNames()
    {
        string names = "";

        for ( int i = 0; i < GetPlayers().Count(); i++ )
        {
            PlayerBase p = GetPlayers()[i];
            if ( !p )
                continue;

            string name = BattleRoyaleKillAttribution.NameOfPlayer( p );
            if ( name == "" )
                continue;

            if ( names != "" )
                names = names + ", ";

            names = names + name;
        }

        return names;
    }

    //! The win screen's payload. Same figures the Last Match card books for this player at KickWinner.
    protected void SendWinScreen(PlayerBase player_winner, PlayerIdentity winner_identity, string squad_names)
    {
        BattleRoyaleMatchStats stats = BattleRoyaleMatchStats.GetInstance();

        string uid = player_winner.player_steamid;
        if ( uid == "" )
            uid = winner_identity.GetPlainId();

        int kills = BattleRoyaleKillLedger.GetInstance().GetKills( uid );
        int damage = stats.GetDamage( uid );
        int hits = stats.GetHits( uid );
        int survived_s = stats.GetElapsedSeconds();
        bool next_match = BattleRoyaleInPlaceReset.KeepsPlayersConnected();
        int kick_ms = i_SecondsTillKick * 1000;

        Param9<int, bool, int, int, int, int, string, bool, int> data = new Param9<int, bool, int, int, int, int, string, bool, int>( stats.GetFieldSize(), stats.IsGrouped(), kills, damage, hits, survived_s, squad_names, next_match, kick_ms );
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", data, true, winner_identity );

        string line = "[Win State] win screen kills=" + kills;
        line = line + " dmg=" + damage;
        line = line + " hits=" + hits;
        line = line + " alive=" + survived_s + "s";
        line = line + " field=" + stats.GetFieldSize();
        BattleRoyaleUtils.Info( line );
    }

    void HandleWinner(PlayerBase player_winner, string squad_names)
    {
        // Send notification
        MessagePlayerUntranslated(player_winner, "STR_BR_ANNOUNCEMENT_WINNER");

        //--- A NULL identity here would make SendRPC broadcast the win screen to every client.
        PlayerIdentity winner_identity = player_winner.GetIdentity();
        if ( winner_identity )
        {
            SendWinScreen( player_winner, winner_identity, squad_names );
        }

        //--- Spawn chickens. Winner winner. The ring, the water rejection, the surface drop and
        //--- the missing NULL check all live in BattleRoyaleChickens now - the lobby drips birds
        //--- into the waiting area with the same helper, and two copies of this would drift.
        //--- Tracked so an in-place reset can delete them; a process restart used to do that for free.
        if( !s_WinChickens )
            s_WinChickens = new array<EntityAI>();

        BattleRoyaleChickens.SpawnBatch(player_winner.GetPosition(), BR_WIN_CHICKEN_INNER_M, BR_WIN_CHICKEN_OUTER_M, BR_WIN_CHICKEN_COUNT, s_WinChickens);
    }

    //--- Every win-screen chicken spawned in this process. Static, so the in-place reset can reach it
    //--- after the state that spawned them is gone.
    protected static ref array<EntityAI> s_WinChickens;

    //--- In-place reset (BattleRoyaleInPlaceReset): delete the win-screen birds still standing.
    static int DeleteWinChickens()
    {
        if( !s_WinChickens )
            return 0;

        int deleted = 0;

        for( int i = s_WinChickens.Count() - 1; i >= 0; i-- )
        {
            EntityAI chicken = s_WinChickens.Get(i);
            if( chicken && !chicken.IsSetForDeletion() )
            {
                GetGame().ObjectDelete( chicken );
                deleted++;
            }
        }

        s_WinChickens.Clear();
        return deleted;
    }

    void KickWinner()
    {
		//--- Walked backwards because RemovePlayer() mutates the very array being indexed:
		//--- m_Players.RemoveItem() is the unordered variant, so it swaps the last element into the
		//--- freed slot. Going forwards that moved an unvisited winner into an already-passed index
		//--- while the loop kept advancing, so part of a winning team never reached DisconnectPlayer
		//--- and stayed connected until the process exited. Iterating from the end makes every
		//--- removal target the current last element, which is a plain shrink.
		//---
		//--- This was near-unreachable while only a solo player could win. Parties ship with the mod
		//--- now, so every team victory goes through it.
		//---
		//--- With the in-place reset keeping players connected, the winners stay: RemovePlayer still
		//--- records their result, and BattleRoyaleInPlaceReset hands them a lobby body next.
		bool keep_winners = BattleRoyaleInPlaceReset.KeepsPlayersConnected();

		for ( int k = GetPlayers().Count() - 1; k >= 0; k-- )
		{
			PlayerBase winner = GetPlayers()[k];
			if( !winner )
				continue;
			if( !winner.GetIdentity() )
				continue;

			RemovePlayer(winner); //disconnect does not trigger RemovePlayer !

			if( !keep_winners )
				GetGame().DisconnectPlayer( winner.GetIdentity() );
		}

		//--- The RemovePlayer calls above are what actually record the winners, so this is the first
		//--- point their results exist. Still only an optimisation over the restart-state flush.
		BattleRoyaleLeaderboard.GetInstance().Flush();

        //--- Deferred: this is a timer callback, i.e. inside TimerQueue.Tick. See
        //--- BattleRoyaleState.DeactivateDeferred().
        DeactivateDeferred();
    }
}
