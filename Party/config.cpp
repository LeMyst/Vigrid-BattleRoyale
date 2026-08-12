/**
 *  Vigrid Party - standalone party/team manager.
 *
 *  This folder is its own PBO (party.pbo, prefix Vigrid-BattleRoyale\Party) because the build
 *  packs every folder holding a config.cpp with no ancestor config.cpp. Keep exactly ONE
 *  config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Party/ may reference a BattleRoyale* symbol. The Battle Royale
 *  mod consumes this addon through VigridPartyAPI only, and every one of its call sites is
 *  wrapped in #ifdef VIGRID_PARTY. That keeps a later extraction into a standalone @Vigrid-Party
 *  mod a build-plumbing job rather than a rewrite - and it keeps `config.cpp.disabled` working as
 *  a one-rename kill switch.
 */
class CfgPatches
{
    class Vigrid_Party
    {
        units[] = {};
        weapons[] = {};
        requiredVersion = 0.1;
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Scripts",
            "JM_CF_Scripts"
        };
    };
};

class CfgMods
{
    class Vigrid_Party
    {
        dir = "Party";
        credits = "Myst";
        type = "mod";
        name = "Vigrid Party";

        //--- A second inputs= alongside the Battle Royale one is supported: @DayZ-Expansion ships
        //--- three PBOs that each declare their own, loaded at the same time as this mod's.
        inputs = "Vigrid-BattleRoyale/Party/Data/Inputs.xml";

        dependencies[] =
        {
            "Game",
            "World",
            "Mission"
        };

        //--- Consumed by the Battle Royale mod to guard its call sites. defines[] are visible
        //--- across CfgMods entries - the same mechanism that made #ifdef Carim work.
        defines[] =
        {
            "VIGRID_PARTY"
        };

        //--- Only declare a script module once its folder actually exists: the engine registers
        //--- files by directory, and pointing a module at a missing folder is a load-time error.
        class defs
        {
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Party/Scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Party/Scripts/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Party/Scripts/5_Mission"
                };
            };
        };
    };
};
