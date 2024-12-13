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
	
		string winner_name = "<NO:WINNER>";
		if(GetPlayers().Count() > 0)
		{
			for ( int k = 0; k < GetPlayers().Count(); k++ )
			{
				PlayerBase winner = GetPlayers()[k];
				PlayerIdentity identity = winner.GetIdentity();
				if(identity)
				{
					winner_name = identity.GetName();
					Print("[Win State] Winner!");
					Print(identity.GetName());
					Print(identity.GetFullName());
					Print(identity.GetId());
					Print(identity.GetPlainId());
				}
				HandleWinner(winner);
			}
		}

		m_KickTimer = AddTimer(i_SecondsTillKick, this, "KickWinner", NULL, false);
	}

    override string GetName()
    {
        return DAYZBR_SM_WIN_NAME;
    }

    override void Deactivate()
    {
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
        // Send notification
        MessagePlayer(player_winner, "Congratulations! You won Battle Royale!");

        // Show win screen
		GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "ShowWinScreen", NULL, true, player_winner.GetIdentity() );

        BattleRoyaleConfig m_Config = BattleRoyaleConfig.GetConfig();
        BattleRoyaleServerData m_ServerData = m_Config.GetServerData();

        // Send win webhook
        BattleRoyaleServer br_instance = BattleRoyaleServer.GetInstance();
		WinWebhook winWebhook = new WinWebhook( m_ServerData.webhook_jwt_token );
		winWebhook.Send( br_instance.match_uuid, player_winner.GetIdentity().GetPlainId() );

        // Send score webhook
		BattleRoyaleUtils.Trace("ScoreWebhook: Sending winner score");
		ScoreWebhook scoreWebhook = new ScoreWebhook( m_ServerData.webhook_jwt_token );
		scoreWebhook.Send( br_instance.match_uuid, player_winner.GetIdentity().GetPlainId(), player_winner.GetBRPosition() );

        // Spawn chickens
		ref array<string> chickens = {"Animal_GallusGallusDomesticus", "Animal_GallusGallusDomesticusF_Brown", "Animal_GallusGallusDomesticusF_Spotted", "Animal_GallusGallusDomesticusF_White"};

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
		if(GetPlayers().Count() > 0)
		{
			for ( int k = 0; k < GetPlayers().Count(); k++ )
			{
				PlayerBase winner = GetPlayers()[k];
				if(winner && winner.GetIdentity() )
				{
					RemovePlayer(winner); //disconnect does not trigger RemovePlayer !
					GetGame().DisconnectPlayer( winner.GetIdentity() );
				}

			}
		}

        Deactivate();
    }
}
