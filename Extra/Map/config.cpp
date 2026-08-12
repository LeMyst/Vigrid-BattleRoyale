/**
 *  Vigrid Map - standalone in-game map, world markers and minimap.
 *
 *  This folder is its own PBO (extra_map.pbo, prefix Vigrid-BattleRoyale\Extra\Map) because the
 *  build packs every folder holding a config.cpp with no ancestor config.cpp. Keep exactly ONE
 *  config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/Map/ may reference a BattleRoyale* symbol. The Battle
 *  Royale mod pushes its zone geometry in through VigridMapAPI and nothing else, and every one of
 *  its call sites is wrapped in #ifdef VIGRID_MAP. Party is reached only through VigridMapTeam,
 *  whose bodies are the addon's only #ifdef VIGRID_PARTY code. Both keep `config.cpp.disabled`
 *  working as a one-rename kill switch, in either direction.
 *
 *  The only place the host mod's name survives is the asset path prefix: CI1.bat builds every
 *  PBO's prefix as <PrefixLinkRoot>\<folder> and this build has no $PBOPREFIX$ override. That path
 *  appears exactly once in the scripts, as VIGRID_MAP_PREFIX in VigridMapConstants.c.
 */
class CfgPatches
{
    class Vigrid_Map
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
    class Vigrid_Map
    {
        dir = "Extra/Map";
        credits = "Myst";
        type = "mod";
        name = "Vigrid Map";

        //--- A third inputs= alongside the Battle Royale and Party ones is supported:
        //--- @DayZ-Expansion ships three PBOs that each declare their own.
        inputs = "Vigrid-BattleRoyale/Extra/Map/Data/Inputs.xml";

        dependencies[] =
        {
            "Game",
            "World",
            "Mission"
        };

        //--- VIGRID_MAP is consumed by the Battle Royale mod to guard its call sites. defines[] are
        //--- visible across CfgMods entries - the same mechanism that made #ifdef Carim work - and
        //--- also to this addon's own scripts, which is what VIGRID_MAP_MINIMAP relies on.
        //---
        //--- TO BUILD WITHOUT THE MINIMAP: comment out the "VIGRID_MAP_MINIMAP" line below and
        //--- rebuild. That removes VigridMapMinimap entirely - the class, the widgets and the N
        //--- keybind handler - while leaving the fullscreen map untouched. It is the hard switch;
        //--- the runtime pair (minimap_allowed in map_settings.json for the admin, minimap_enabled
        //--- in map_client.json for the player) still exists and is unaffected.
        //---
        //--- Two things survive a minimap-less build on purpose: the RPC field and server setting,
        //--- because a client build flag must not change the wire format; and the N entry in
        //--- Data/Inputs.xml, because XML cannot be conditional, so it still lists under
        //--- Options > Controls doing nothing.
        //---
        //--- TO BUILD WITHOUT THE COMPASS: comment out the "VIGRID_MAP_COMPASS" line below. Same
        //--- shape as the minimap switch and with the same two exceptions - compass_allowed stays on
        //--- the wire and in map_settings.json, and the K entry stays in Data/Inputs.xml. The
        //--- fullscreen map and the minimap are untouched either way.
        defines[] =
        {
            "VIGRID_MAP",
            "VIGRID_MAP_MINIMAP",
            "VIGRID_MAP_COMPASS"
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
                    "Vigrid-BattleRoyale/Extra/Map/Scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/Map/Scripts/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/Map/Scripts/5_Mission"
                };
            };
        };
    };
};
