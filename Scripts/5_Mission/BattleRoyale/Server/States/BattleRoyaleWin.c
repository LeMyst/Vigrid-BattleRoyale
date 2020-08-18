#define BR_BETA_LOGGING

class BattleRoyaleWin extends BattleRoyaleState
{
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
			MessagePlayer(winner, "Congratulations! You won Battle Rooooyale!");
		}
		BattleRoyaleAPI.GetAPI().ServerFinish(winner_name); //report winner to api
		
		//TODO: report match data to leaderboards

		//TODO: start server shutdown process

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
		return false; //win state is never complete (it is an end state)
	}
}