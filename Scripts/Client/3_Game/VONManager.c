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

	override void SetMaxVolume(int max_level)
	{
		max_voice_level = max_level;
	}

	/**
	 *  Diagnostic only - no behaviour depends on this.
	 *
	 *  Vanilla builds its own "who is speaking" notification list out of these two engine events
	 *  (VONManagerImplementation.OnEvent -> DayZGame.AddVoiceNotification -> NotificationUI), and
	 *  NotificationUI is constructed on every platform. But every vanilla call that clears that list
	 *  is #ifdef PLATFORM_CONSOLE, which suggests the engine may only fire the events on console -
	 *  and script cannot tell either way.
	 *
	 *  BattleRoyaleSpeakingList deliberately does not depend on the answer; it samples
	 *  IsPlayerSpeaking() instead. If these lines ever show up in a trace-level .rpt on PC, the
	 *  events are a cheaper source that arrives already filtered by the engine, and the panel could
	 *  be re-pointed at them.
	 *
	 *  Trace is a runtime level check, so this costs a comparison per VON event on normal builds.
	 */
	override void OnEvent(EventType eventTypeId, Param params)
	{
		super.OnEvent(eventTypeId, params);

		if (eventTypeId == VONStartSpeakingEventTypeID)
		{
			VONStartSpeakingEventParams start_params;
			if (Class.CastTo(start_params, params))
				BattleRoyaleUtils.Trace(string.Format("VON probe: start speaking name=%1 uid=%2", start_params.param1, start_params.param2));
		}

		if (eventTypeId == VONStopSpeakingEventTypeID)
		{
			VONStopSpeakingEventParams stop_params;
			if (Class.CastTo(stop_params, params))
				BattleRoyaleUtils.Trace(string.Format("VON probe: stop speaking name=%1 uid=%2", stop_params.param1, stop_params.param2));
		}
	}
}
