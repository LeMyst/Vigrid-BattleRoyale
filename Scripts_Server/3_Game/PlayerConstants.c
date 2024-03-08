#ifdef SERVER
modded class PlayerConstants
{
    static const float HEALTH_REGEN_MIN =           0.005   * BATTLEROYALE_HEALTH_REGEN_MODIFIER;
    static const float HEALTH_REGEN_MAX =           0.03    * BATTLEROYALE_HEALTH_REGEN_MODIFIER;
    static const float BLOOD_REGEN_RATE_PER_SEC =   0.4     * BATTLEROYALE_BLOOD_REGEN_MODIFIER;
};
#endif