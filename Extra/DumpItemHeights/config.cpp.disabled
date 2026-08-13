/**
 *  DumpItemHeights - writes every loot item's bounding box to a CSV, once, on an offline Diag client.
 *
 *  This folder is its own PBO because the build packs every folder holding a config.cpp with no
 *  ancestor config.cpp. Keep exactly ONE config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/DumpItemHeights/ may reference a BattleRoyale* symbol. The
 *  addon hooks the vanilla MissionGameplay directly, so it works on any DayZ install with or
 *  without Battle Royale loaded - and it keeps `config.cpp.disabled` working as a one-rename kill
 *  switch.
 *
 *  WHY THIS EXISTS: a loot item's vertical extent is needed to set <point height="..."/> in
 *  mapgroupproto.xml, and that number lives in the .p3d rather than in config. There is no
 *  boundingSphere / bounding box / envCargo key anywhere in P:\dz, and itemSize[] is a 2D
 *  inventory-grid slot count - vanilla's own reader of it (itembase.c GetItemSize) is commented out
 *  and returns 1. So the only way to get the number is to instantiate each item and ask the engine.
 *
 *  THIS IS A THROWAWAY TOOL. It is meant to be built once, run once, and then disabled by renaming
 *  this file to config.cpp.disabled.
 */
class CfgPatches
{
    //--- Underscored so the config class name never collides with the script class DumpItemHeights,
    //--- the same reason RandomMenuGear/config.cpp uses Random_Menu_Gear. The PBO name comes from the
    //--- folder path, not from here, so this stays extra_dumpitemheights.pbo.
    class Dump_Item_Heights
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
    class Dump_Item_Heights
    {
        dir = "Extra/DumpItemHeights";
        credits = "Myst";
        type = "mod";
        name = "Dump Item Heights";

        dependencies[] =
        {
            "Mission"
        };

        //--- Nothing consumes this today. Declared for consistency with the other Extra addons, so
        //--- a future call site can guard itself with #ifdef DUMP_ITEM_HEIGHTS.
        defines[] =
        {
            "DUMP_ITEM_HEIGHTS"
        };

        //--- Only declare a script module once its folder actually exists: the engine registers
        //--- files by directory, and pointing a module at a missing folder is a load-time error.
        class defs
        {
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/DumpItemHeights/Scripts/5_Mission"
                };
            };
        };
    };
};
