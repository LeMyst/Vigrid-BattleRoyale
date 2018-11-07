modded class PlayerBase
{
	bool FORCE_FADE = false;

	float timeTillNextHealTick = 0;
	float timeTillNextDmgTick = 0;
	
	string my_round;
	
	ref BattleRoyaleBase BR_BASE;
	
	override void OnScheduledTick(float deltaTime)
	{
		super.OnScheduledTick(deltaTime);
		
		if( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
		{
			if(BR_BASE)
				BR_BASE.OnPlayerTick(this, deltaTime);
		}
	}
	
	override void EEKilled( Object killer )
	{
		super.EEKilled(killer);
		
		if( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
		{
			if(BR_BASE)
				BR_BASE.OnPlayerKilled(this,killer);
		}
	}
}