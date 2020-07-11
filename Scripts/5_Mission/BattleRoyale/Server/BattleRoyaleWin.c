class BattleRoyaleWin extends BattleRoyaleState
{
	//TODO: state functionality for winners!
	override void Activate()
	{
		super.Activate();
		if(GetPlayers().Count() > 0)
		{
			MessagePlayer(GetPlayers()[0],"Congratulations! You won Battle Rooooyale!");
		}

		//TODO: start server shutdown process
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