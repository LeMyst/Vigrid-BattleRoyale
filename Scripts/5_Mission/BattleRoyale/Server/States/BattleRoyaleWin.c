class BattleRoyaleWin extends BattleRoyaleState
{
	int i_SecondsTillKick;
	PlayerBase winner;
	bool complete;

	protected ref Timer m_KickTimer;

	void BattleRoyaleWin()
	{
		i_SecondsTillKick = 15; //TODO: config this
		complete = false;
	}

	//TODO: state functionality for winners!
	override void Activate()
	{
		super.Activate();

		BattleRoyaleServer.Cast( GetBR() ).GetMatchData().SetEnd(GetGame().GetTime()); //match end time logging

		ref MatchData match_data = BattleRoyaleServer.Cast( GetBR() ).GetMatchData();
		
		string winner_name = "<NO:WINNER>";
		if(GetPlayers().Count() > 0)
		{
			winner = GetPlayers()[0];
			PlayerIdentity identity = winner.GetIdentity();
			if(identity)
			{
				winner_name = identity.GetName();
				Print("[Win State] Winner!");
				Print(identity.GetName());
				Print(identity.GetFullName());
				Print(identity.GetId());
				Print(identity.GetPlainId());
				match_data.CreateWinner( identity.GetPlainId() );
			}
			HandleWinner(winner);
		}

		//log match end weather
		float rain_intensity = GetGame().GetWeather().GetRain().GetActual();
		float fog_intensity = GetGame().GetWeather().GetFog().GetActual();
		match_data.SetWorldWeather( rain_intensity, fog_intensity );

		if(BattleRoyaleAPI.GetAPI().ShouldUseApi())
		{
			BattleRoyaleAPI.GetAPI().ServerFinish( winner_name ); //report winner to api
			
			//report match data to leaderboard

			BattleRoyaleAPI.GetAPI().SubmitMatchData( match_data );
		}
		//TODO: write match data to disk possible for private servers?

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
		{
			Deactivate();
		}
		return super.IsComplete(); //go to restart state when player disconnects
	}

	void HandleWinner(PlayerBase winner)
	{
		MessagePlayer(winner, "Congratulations! You won Battle Royale!");
	}
	void KickWinner()
	{
		if(winner && winner.GetIdentity() )
		{
			RemovePlayer(winner); //disconnect does not trigger RemovePlayer !
			GetGame().DisconnectPlayer( winner.GetIdentity() );
		}

		Deactivate();
	}
}