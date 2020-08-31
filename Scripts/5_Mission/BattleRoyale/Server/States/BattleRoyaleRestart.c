class BattleRoyaleRestart extends BattleRoyaleState
{
	override void Activate()
	{
		super.Activate();
        Print("[Restart State] Restarting!");
		
		m_CallQueue.CallLater(this.Shutdown, 10000, false);
	}
	override string GetName()
	{
		return DAYZBR_SM_RESTART_NAME;
	}
	override void Deactivate()
	{
		Error("RESTART STATE WAS DEACTIVATED!");
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