#ifdef SERVER
modded class WorldData
{
	override protected float CalcBaseEnvironmentTemperature( float monthday, float daytime )
	{
		for (int tempIdx = 0; tempIdx < 11; tempIdx++)
		{
			if(m_MaxTemps[tempIdx] > 18.5)
				m_MaxTemps[tempIdx] = 18.5;
		}

		return super.CalcBaseEnvironmentTemperature( monthday, daytime );
	}
}
