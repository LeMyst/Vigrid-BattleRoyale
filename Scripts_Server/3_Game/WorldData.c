#ifdef SERVER
modded class WorldData
{
	override protected float CalcBaseEnvironmentTemperature( float monthday, float daytime )
	{
		for (int tempIdx = 0; tempIdx < m_MaxTemps.Count(); tempIdx++)
		{
			if(m_MaxTemps[tempIdx] > 18)
				m_MaxTemps[tempIdx] = 18;
		}

		super.CalcBaseEnvironmentTemperature( monthday, daytime );
	}
}
