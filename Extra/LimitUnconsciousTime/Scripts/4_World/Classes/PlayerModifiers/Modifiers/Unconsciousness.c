#ifdef SERVER
modded class UnconsciousnessMdfr
{
	// TODO: Move these constants to a config file
	const float UNCONSCIOUSNESS_MAX_TIME = 5.0; // in seconds
	const float SHOCK_DAMAGE_AFTER_UNCONSCIOUSNESS = PlayerConstants.CONSCIOUS_THRESHOLD + 0.1; // Set health to conscious threshold

	override void Init()
	{
		BattleRoyaleUtils.Trace("UnconsciousnessMdfr::Init");
		super.Init();
	}

	override bool ActivateCondition(PlayerBase player)
	{
		if(super.ActivateCondition(player))
		{
			// Save the time when the player became unconscious
			player.m_UnconsciousStartTime = GetGame().GetTime();
			return true;
		}

		return false;
	}

	override bool DeactivateCondition(PlayerBase player)
	{
		if (GetGame().GetTime() - player.m_UnconsciousStartTime > (UNCONSCIOUSNESS_MAX_TIME * 1000))
		{
			player.SetHealth("", "Shock", SHOCK_DAMAGE_AFTER_UNCONSCIOUSNESS);
			return true;  // Deactivate the modifier
		}
		else
		{
			return super.DeactivateCondition(player);
		}
	}
}