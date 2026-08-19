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

		string winner_name = "<NO:WINNER>";
		if(GetPlayers().Count() > 0)
		{
			for ( int k = 0; k < GetPlayers().Count(); k++ )
			{
				PlayerBase winner = GetPlayers()[k];
				PlayerIdentity identity = winner.GetIdentity();
				if(identity)
				{
					winner_name = BattleRoyaleNameService.ResolveIdentity(identity);
					BattleRoyaleUtils.Trace("[Win State] Winner!");
					BattleRoyaleUtils.Trace(identity.GetName());
					BattleRoyaleUtils.Trace(identity.GetFullName());
					BattleRoyaleUtils.Trace(identity.GetId());
					BattleRoyaleUtils.Trace(identity.GetPlainId());
				}
				HandleWinner(winner);
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

    void HandleWinner(PlayerBase player_winner)
    {
        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleServerData m_ServerData = m_Config.GetServerData();

        // Send notification
        MessagePlayerUntranslated(player_winner, "STR_BR_ANNOUNCEMENT_WINNER");

        // Show win screen
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", NULL, true, player_winner.GetIdentity() );

		if ( m_ServerData.enable_vigrid_api )
		{
			// Send win webhook
			BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
			WinWebhook winWebhook = new WinWebhook( m_ServerData.webhook_jwt_token );
			winWebhook.Send( br_instance.match_uuid, player_winner.GetIdentity().GetPlainId() );

			// Send score webhook
			BattleRoyaleUtils.Trace("ScoreWebhook: Sending winner score");
			ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
			scoreWebhook.Send( br_instance.match_uuid, player_winner.GetIdentity().GetPlainId(), player_winner.GetBRPosition() );
		}

        // Spawn chickens
		array<string> chickens = {"Animal_GallusGallusDomesticus", "Animal_GallusGallusDomesticusF_Brown", "Animal_GallusGallusDomesticusF_Spotted", "Animal_GallusGallusDomesticusF_White"};

		for (int j = 0; j < 10; j++)
		{
			vector position;

			vector player_position = player_winner.GetPosition();
			float radius = Math.RandomFloat(1, 5);
			float angle = Math.RandomFloat(0, 360) * Math.DEG2RAD;
			position[0] = player_position[0] + ( radius * Math.Cos(angle) );
			position[2] = player_position[2] + ( radius * Math.Sin(angle) );
			position[1] = GetGame().SurfaceY(position[0], position[2]);

			EntityAI chicken = EntityAI.Cast(GetGame().CreateObject( chickens.GetRandomElement(), position, false, true ));

			float dir = Math.RandomFloat(0, 360);
			vector chickenDir = vector.YawToVector(dir);
			chicken.SetDirection( Vector(chickenDir[0], 0, chickenDir[1]) );
		}
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
		for ( int k = GetPlayers().Count() - 1; k >= 0; k-- )
		{
			PlayerBase winner = GetPlayers()[k];
			if( !winner )
				continue;
			if( !winner.GetIdentity() )
				continue;

			RemovePlayer(winner); //disconnect does not trigger RemovePlayer !
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
