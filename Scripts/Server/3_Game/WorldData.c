#ifdef SERVER
modded class WorldData
{
	override protected float CalcBaseEnvironmentTemperature( float monthday, float daytime )
	{
		//--- m_MaxTemps is float[12], one entry per month - December included.
		for (int tempIdx = 0; tempIdx < 12; tempIdx++)
		{
			if(m_MaxTemps[tempIdx] > 18.5)
				m_MaxTemps[tempIdx] = 18.5;
		}

		return super.CalcBaseEnvironmentTemperature( monthday, daytime );
	}
}
#endif
