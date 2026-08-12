/**
 *  SafeZone - a narrow, self-contained truce switch. While active: a trigger pull does nothing, and
 *  damage inflicted by another player is discarded. Nothing else changes.
 *
 *  Deliberately NOT a clone of DayZ Expansion's safezone. Expansion turns a safezone into a general
 *  pacifier - it calls hic.OverrideRaise(true, false), which kills weapon raise and therefore ADS
 *  for every item including melee, and it hard-returns false from HandleFightLogic, so melee swings
 *  do not even play. This addon leaves raise, ADS and melee animation completely alone: players
 *  waiting in a lobby can still aim and still punch each other, they just cannot do harm.
 *
 *  This folder is its own PBO because the build packs every folder holding a config.cpp with no
 *  ancestor config.cpp. Keep exactly ONE config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/SafeZone/ may reference a BattleRoyale* symbol. The host mod
 *  consumes this addon through VigridSafeZoneAPI only, and every one of its call sites is wrapped in
 *  #ifdef VIGRID_SAFEZONE. The addon hooks vanilla PlayerBase and WeaponManager directly, so it
 *  works on any DayZ server with or without Battle Royale loaded. That keeps a later extraction into
 *  a standalone @Vigrid-SafeZone mod a build-plumbing job rather than a rewrite - and it keeps
 *  `config.cpp.disabled` working as a one-rename kill switch.
 *
 *  The addon ships no assets, no settings file and no UI text, so unlike KillFeed there is no asset
 *  prefix constant to keep in sync.
 */
class CfgPatches
{
    class VigridSafeZone
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class VigridSafeZone
    {
        dir = "Extra/SafeZone";
        credits = "Myst";
        type = "mod";
        name = "Vigrid Safe Zone";

        dependencies[] =
        {
            "Game",
            "World"
        };

        //--- Consumed by the Battle Royale mod to guard its call sites. defines[] are visible
        //--- across CfgMods entries - the same mechanism that made #ifdef Carim work.
        defines[] =
        {
            "VIGRID_SAFEZONE"
        };

        //--- Only declare a script module once its folder actually exists: the engine registers
        //--- files by directory, and pointing a module at a missing folder is a load-time error.
        //--- There is no 5_Mission folder here on purpose - the addon needs no mission bootstrap.
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/SafeZone/Scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/SafeZone/Scripts/4_World"
                };
            };
        };
    };
};
