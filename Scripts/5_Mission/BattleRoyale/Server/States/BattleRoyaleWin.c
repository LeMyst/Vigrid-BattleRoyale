class BattleRoyaleWin extends BattleRoyaleState
{
	int i_SecondsTillKick;
	PlayerBase winner;
	bool complete;

	void BattleRoyaleWin()
	{
		i_SecondsTillKick = 15; //TODO: config this
		complete = false;
	}

	//TODO: state functionality for winners!
	override void Activate()
	{
		super.Activate();
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
			}
			HandleWinner(winner);
		}
		BattleRoyaleAPI.GetAPI().ServerFinish(winner_name); //report winner to api
		
		//TODO: report match data to leaderboards

		m_CallQueue.CallLater(this.KickWinner, i_SecondsTillKick * 1000, false);
	}
	override string GetName()
	{
		return DAYZBR_SM_WIN_NAME;
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