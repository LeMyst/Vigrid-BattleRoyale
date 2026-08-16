/**
 *  Vigrid Auto-Run - hold a movement speed with no keys held.
 *
 *  This folder is its own PBO (extra_autorun.pbo, prefix Vigrid-BattleRoyale\Extra\AutoRun) because
 *  the build packs every folder holding a config.cpp with no ancestor config.cpp. Keep exactly ONE
 *  config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/AutoRun/ may reference a BattleRoyale* symbol. The host mod
 *  reaches this addon only through VigridAutoRunAPI, and every one of its call sites is wrapped in
 *  #ifdef VIGRID_AUTORUN - which keeps `config.cpp` -> `config.cpp.disabled` working as a one-rename
 *  kill switch, in either direction. It hooks vanilla and nothing else, so it works on any DayZ
 *  server.
 *
 *  The only place the host mod's name survives is the asset path prefix: CI1.bat builds every PBO's
 *  prefix as <PrefixLinkRoot>\<folder> and this build has no $PBOPREFIX$ override. That path appears
 *  exactly once in the scripts, as VIGRID_AUTORUN_PREFIX in VigridAutoRunConstants.c.
 */
class CfgPatches
{
    class Vigrid_AutoRun
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
    class Vigrid_AutoRun
    {
        dir = "Extra/AutoRun";
        credits = "Myst";
        type = "mod";
        name = "Vigrid Auto-Run";

        //--- A fourth inputs= alongside the Battle Royale, Party and Map ones is supported:
        //--- @DayZ-Expansion ships three PBOs that each declare their own.
        inputs = "Vigrid-BattleRoyale/Extra/AutoRun/Data/Inputs.xml";

        dependencies[] =
        {
            "Game",
            "World",
            "Mission"
        };

        //--- VIGRID_AUTORUN is consumed by the host mod to guard its call sites. defines[] are
        //--- visible across CfgMods entries - the same mechanism that made #ifdef Carim work.
        defines[] =
        {
            "VIGRID_AUTORUN"
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
                    "Vigrid-BattleRoyale/Extra/AutoRun/Scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/AutoRun/Scripts/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/AutoRun/Scripts/5_Mission"
                };
            };
        };
    };
};
