#ifdef SERVER
/**
 *  SpawnWithAmmoAndMagazine - the three serverDZ.cfg keys, read once per process.
 *
 *  EEOnCECreate fires once for every item the Central Economy creates. On a cold ChernarusPlus boot
 *  that is 33,921 items, and the original version read BRDisableSpawnWithAmmo for all of them -
 *  before the IsWeapon() test, so every tin can and rag paid for a config lookup - plus BRMinSpawnAmmo
 *  and BRMaxSpawnAmmo again for each weapon. serverDZ.cfg is parsed at boot and never changes under a
 *  running server, so all of that was waste, in the phase that dominates boot time.
 *
 *  Same shape as VigridPreventWeaponRaiseState in the PreventWeaponRaise addon, and for the same
 *  reason. Kept out of `modded class ItemBase` deliberately: SpawnWithBattery mods that class too, and
 *  statics declared on both would collide.
 */
class VigridSpawnAmmoConfig
{
    private static bool s_Resolved;
    private static bool s_Disabled;
    private static int s_Min;
    private static int s_Max;

    private static void Resolve()
    {
        if (s_Resolved) return;
        s_Resolved = true;

        s_Disabled = GetGame().ServerConfigGetInt("BRDisableSpawnWithAmmo") == 1;

        //  Defaults, then the admin's overrides on top - preserving the original precedence exactly:
        //  a positive min raises it, a negative min floors it at 0, a positive max raises it, and max
        //  is never allowed below min. An absent key reads as 0, which leaves the default in place.
        int min_spawn = 1;
        int max_spawn = 2;
        int config_min = GetGame().ServerConfigGetInt("BRMinSpawnAmmo");
        int config_max = GetGame().ServerConfigGetInt("BRMaxSpawnAmmo");

        if (config_min > 0) min_spawn = config_min;
        if (config_min < 0) min_spawn = 0;
        if (config_max > 0) max_spawn = config_max;
        if (max_spawn < min_spawn) max_spawn = min_spawn;

        s_Min = min_spawn;
        s_Max = max_spawn;
    }

    /**
     *  Has the admin switched the addon off? An absent key reads as 0, i.e. leave it active, so a
     *  server that has never heard of this key keeps the behaviour it has today.
     */
    static bool IsDisabled()
    {
        Resolve();
        return s_Disabled;
    }

    static int GetMinSpawn()
    {
        Resolve();
        return s_Min;
    }

    static int GetMaxSpawn()
    {
        Resolve();
        return s_Max;
    }
}
#endif
