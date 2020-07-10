modded class PlayerBase
{
	override void OnScheduledTick(float deltaTime)
	{
		super.OnScheduledTick(deltaTime);
		
		BattleRoyaleBase m_BR = GetBR();
		if(m_BR)
		{
			m_BR.OnPlayerTick(this, deltaTime);
		}
	}
	override void EEKilled( Object killer )
	{
		super.EEKilled( killer );
		
		BattleRoyaleBase m_BR = GetBR();
		if(m_BR)
		{
			m_BR.OnPlayerKilled(this, killer);
		}
	}
}