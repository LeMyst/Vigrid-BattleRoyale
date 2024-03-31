#ifndef SERVER
modded class VONManagerBase
{
	void SetMaxVolume(int max_level);
}

modded class VONManagerImplementation
{
	protected int max_voice_level = VoiceLevelWhisper;

	override void HandleInput(Input inp)
	{
		int oldLevel = GetGame().GetVoiceLevel();
		if (oldLevel == -1) //VoN system not initialized!
			return;

		int newLevel = -1;

		if (inp.LocalPress_ID(UAVoiceDistanceUp,false))
		{
			newLevel = ( oldLevel + 1 ) % ( max_voice_level + 1 );
		}

		if (inp.LocalPress_ID(UAVoiceDistanceDown,false))
		{
			newLevel = oldLevel - 1;
			if (newLevel < VoiceLevelWhisper) //nah...
			{
				newLevel = max_voice_level;
			}
		}

		if (newLevel > -1)
		{
			GetGame().SetVoiceLevel(newLevel);
			UpdateVoiceIcon();
		}

		if ( GetGame().GetVoiceLevel() > max_voice_level )
		{
			GetGame().SetVoiceLevel( max_voice_level ); // Force level to whisper
			UpdateVoiceIcon();
		}
	}

	void SetMaxVolume(int max_level)
	{
		max_voice_level = max_level;
	}
}
