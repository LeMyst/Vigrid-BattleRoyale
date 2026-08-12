/**
 *  KillFeed - standalone kill feed with 3D weapon previews.
 *
 *  This folder is its own PBO because the build packs every folder holding a config.cpp with no
 *  ancestor config.cpp. Keep exactly ONE config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/KillFeed/ may reference a BattleRoyale* symbol. The Battle
 *  Royale mod consumes this addon through KillFeedAPI only, and every one of its call sites is
 *  wrapped in #ifdef KILLFEED. The addon hooks vanilla PlayerBase.EEKilled directly, so it works
 *  on any DayZ server with or without Battle Royale loaded. That keeps a later extraction into a
 *  standalone @KillFeed mod a build-plumbing job rather than a rewrite - and it keeps
 *  `config.cpp.disabled` working as a one-rename kill switch.
 *
 *  The only place the host mod's name survives is the asset path prefix: CI1.bat builds every
 *  PBO's prefix as <PrefixLinkRoot>\<folder> and this build has no $PBOPREFIX$ override. That path
 *  appears exactly once in the scripts, as KILLFEED_PREFIX in KillFeedConstants.c.
 */
class CfgPatches
{
    class KillFeed
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
    class KillFeed
    {
        dir = "Extra/KillFeed";
        credits = "Myst";
        type = "mod";
        name = "Kill Feed";

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
            "KILLFEED"
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
                    "Vigrid-BattleRoyale/Extra/KillFeed/Scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/KillFeed/Scripts/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/KillFeed/Scripts/5_Mission"
                };
            };
        };
    };
};
