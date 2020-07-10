class BattleRoyaleWin extends BattleRoyaleState
{
	//TODO: state functionality for winners!
	override void Activate()
	{
		super.Activate();
	}
	override void Deactivate()
	{
		BRPrint("ERROR! WIN STATE WAS DEACTIVATED!");
		super.Deactivate();
	}
	
	override void IsComplete()
	{
		return false; //win state is never complete (it is an end state)
	}
}