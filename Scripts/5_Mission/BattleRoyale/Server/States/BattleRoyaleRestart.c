#define BR_BETA_LOGGING

class BattleRoyaleRestart extends BattleRoyaleState
{
	override void Activate()
	{
		super.Activate();
        BRPrint("Restarting!");
		GetGame().RequestExit(0);
	}
	override string GetName()
	{
		return "Restart State";
	}
	override void Deactivate()
	{
		BRPrint("ERROR! RESTART STATE WAS DEACTIVATED!");
		super.Deactivate();
	}
	
	override bool IsComplete()
	{
		return false; //win state is never complete (it is an end state)
	}
}