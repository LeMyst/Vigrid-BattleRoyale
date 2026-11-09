modded class CfgGameplayHandler
{
	//--- Unguarded on purpose: the owning client predicts hold-breath drain through this same getter
	//--- (SMDataExponential.Init), so both sides must return the same value.
	override static float GetHoldBreathStaminaModifier()
	{
		return 0.5;
	}
}
