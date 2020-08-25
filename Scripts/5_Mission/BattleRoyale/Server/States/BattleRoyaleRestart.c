#define BR_BETA_LOGGING

class BattleRoyaleRestart extends BattleRoyaleState
{
	override void Activate()
	{
		super.Activate();
        BRPrint("Restarting!");
		
		m_CallQueue.CallLater(this.Shutdown, 10000, false);
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

	void Shutdown()
	{
		GetGame().RequestExit(0);
	}
}