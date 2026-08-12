/**
 *  RandomMenuGear - dresses the main-menu intro character in random gear every time the menu shows.
 *
 *  This folder is its own PBO because the build packs every folder holding a config.cpp with no
 *  ancestor config.cpp. Keep exactly ONE config.cpp here and do not nest another one below it.
 *
 *  DISCIPLINE RULE: nothing under Extra/RandomMenuGear/ may reference a BattleRoyale* symbol. The
 *  addon hooks the vanilla IntroSceneCharacter and MainMenu directly, so it works on any DayZ
 *  install with or without Battle Royale loaded. That keeps a later extraction into a standalone
 *  mod a build-plumbing job rather than a rewrite - and it keeps `config.cpp.disabled` working as a
 *  one-rename kill switch.
 *
 *  WHY THIS EXISTS: the menu character usually renders naked because the local character save is
 *  broken - vanilla either restores it through the engine-native MenuData.CreateCharacterPerson()
 *  or falls back to MenuDefaultCharacterData.EquipDefaultCharacter() with an empty attachments map.
 *  This addon deliberately does NOT fix the save. It decorates the spawned menu object only.
 */
class CfgPatches
{
    //--- Underscored so the config class name never collides with the script class RandomMenuGear,
    //--- the same reason Party/config.cpp uses Vigrid_Party against its script class VigridParty.
    //--- The PBO name comes from the folder path, not from here, so this stays Extra_RandomMenuGear.
    class Random_Menu_Gear
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
    class Random_Menu_Gear
    {
        dir = "Extra/RandomMenuGear";
        credits = "Myst";
        type = "mod";
        name = "Random Menu Gear";

        dependencies[] =
        {
            "World",
            "Mission"
        };

        //--- Nothing consumes this today. Declared for consistency with the other Extra addons, so
        //--- a future call site can guard itself with #ifdef RANDOM_MENU_GEAR.
        defines[] =
        {
            "RANDOM_MENU_GEAR"
        };

        //--- Only declare a script module once its folder actually exists: the engine registers
        //--- files by directory, and pointing a module at a missing folder is a load-time error.
        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/RandomMenuGear/Scripts/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/RandomMenuGear/Scripts/5_Mission"
                };
            };
        };
    };
};
