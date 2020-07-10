
class BattleRoyaleRound extends BattleRoyaleState 
{
	ref BattleRoyaleState m_PreviousSate; 
	int i_RoundTimeInSeconds; 
	bool b_TimeUp;
	//If this is NULL, we assume previous state is debug 
	//a battle royale round represents a playing state with a play area
	/*
		
	
	*/
	void BattleRoyaleRound(ref BattleRoyaleState previous_state)
	{
		m_PreviousSate = previous_state;
		b_TimeUp = false;
		i_RoundTimeInSeconds = 60*5; // 5 minutes in seconds (35 min game)
		Init();
	}
	
	
	void Init()
	{
		
	}
	override void Activate()
	{
		//we just activated this round (players not yet transfered from previous state)
		m_CallQueue.CallLater(this.OnRoundTimeUp, i_RoundTimeInSeconds * 1000); //set timer to end round after X seconds
		super.Activate();
	}
	override void Deactivate()
	{
		//we just deactivated this round (players not yet transfered from previous state)
		super.Deactivate();
	}
	
	override void IsComplete() //return true when this state is complete & ready to transfer to the next state
	{
		//todo: transition timer
		return b_TimeUp;
	}
	
	void OnPlayerKilled(PlayerBase player, Object killer)
	{
		
	}
	
	void OnRoundTimeUp()
	{
		b_TimeUp = true;
	}
}