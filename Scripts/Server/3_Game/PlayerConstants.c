#ifdef SERVER
modded class PlayerConstants
{
	static const float HEALTH_REGEN_MIN					= 0.005	* BATTLEROYALE_HEALTH_REGEN_MODIFIER;	//health regen rate at BLOOD_THRESHOLD_FATAL blood level
	static const float HEALTH_REGEN_MAX					= 0.03	* BATTLEROYALE_HEALTH_REGEN_MODIFIER;	//health regen rate at MAXIMUM blood level
	static const float BLOOD_REGEN_RATE_PER_SEC			= 0.3	* BATTLEROYALE_BLOOD_REGEN_MODIFIER;	//base amount of blood regenerated per second
};
#endif
