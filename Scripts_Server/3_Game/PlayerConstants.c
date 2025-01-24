#ifdef SERVER
modded class PlayerConstants
{
    static const float HEALTH_REGEN_MIN					= 0.005	* BATTLEROYALE_HEALTH_REGEN_MODIFIER;	//health regen rate at BLOOD_THRESHOLD_FATAL blood level
	static const float HEALTH_REGEN_MAX					= 0.03	* BATTLEROYALE_HEALTH_REGEN_MODIFIER;	//health regen rate at MAXIMUM blood level
    static const float BLOOD_REGEN_RATE_PER_SEC			= 0.3	* BATTLEROYALE_BLOOD_REGEN_MODIFIER;	//base amount of blood regenerated per second

	static const float UNCONSCIOUS_THRESHOLD			= 25.0	* BATTLEROYALE_UNCONSCIOUS_MODIFIER;	//player goes unconscious when we get under this threshold
	static const float CONSCIOUS_THRESHOLD				= 50.0	* BATTLEROYALE_UNCONSCIOUS_MODIFIER;	//player regains consciousness when he gets above this threshold

	static const float SHOCK_REFILl_UNCONSCIOUS_SPEED	= 1.0	* BATTLEROYALE_SHOCK_REFILL_SPEED;		//shock refill speed when the player is unconscious
};
#endif
