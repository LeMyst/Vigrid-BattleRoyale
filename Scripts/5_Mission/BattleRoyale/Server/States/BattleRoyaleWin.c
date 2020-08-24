#define BR_BETA_LOGGING

class BattleRoyaleWin extends BattleRoyaleState
{
	int i_SecondsTillKick;

	void BattleRoyaleWin()
	{
		i_SecondsTillKick = 15; //TODO: config this
	}

	//TODO: state functionality for winners!
	override void Activate()
	{
		super.Activate();
		PlayerBase winner;
		string winner_name = "<NO:WINNER>";
		if(GetPlayers().Count() > 0)
		{
			winner = GetPlayers()[0];
			PlayerIdentity identity = winner.GetIdentity();
			if(identity)
			{
				winner_name = identity.GetName();
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
		return "Win State";
	}
	override void Deactivate()
	{
		BRPrint("ERROR! WIN STATE WAS DEACTIVATED!");
		super.Deactivate();
	}
	
	override bool IsComplete()
	{
		return GetPlayers().Count() == 0; //go to restart state when player disconnects
	}

	void HandleWinner(PlayerBase winner)
	{
		MessagePlayer(winner, "Congratulations! You won Battle Royale!");
	}
	void KickWinner()
	{
		if(winner)
		{
			GetGame().DisconnectPlayer( winner.GetIdentity() );
		}
	}
}