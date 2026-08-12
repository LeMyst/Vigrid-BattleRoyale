class CfgPatches
{
    class BattleRoyale_Scripts_Client
    {
        requiredAddons[]=
        {
            "DZ_Data",
            "DZ_Scripts",
            "DayZExpansion_Scripts"
        };
    };
};

class CfgMods
{
    class DZ_BattleRoyale_Client
    {
        credits = "Kegan - Modified by Myst";
        creditsJson = "Vigrid-BattleRoyale/Data/credits.json";
        type = "mod";
        inputs = "Vigrid-BattleRoyale/Data/Inputs.xml";
        name = "DayZ BattleRoyale";

        picture = "Vigrid-BattleRoyale/GUI/textures/Mod_Logo.paa";
        logo = "Vigrid-BattleRoyale/GUI/textures/Mod_Logo.paa";
        logoSmall = "Vigrid-BattleRoyale/GUI/textures/Mod_Logo.paa";
        logoOver = "Vigrid-BattleRoyale/GUI/textures/Mod_Logo.paa";

        dependencies[]=
        {
            "Game",
            "World",
            "Mission"
        };

		defines[]=
		{
			"DAYZ_BATTLEROYALE",
			//--- Draw the zone circles into DayZ Expansion's map as well as this mod's own.
			//--- Requires @DayZ-Expansion-Navigation to be loaded: ExpansionMapMenu lives in that
			//--- PBO, and with it absent the guarded code fails to compile the Mission module
			//--- outright rather than degrading. Off because the Vigrid map addon replaces it.
			//"EXPANSION_MAP_ZONES",
			//"BR_TRACE_ENABLED",
			//--- Note the missing trailing comma below: uncommenting MOVING_ZONE and the line after
			//--- it together is a rapify syntax error, not just a dead define.
			//"MOVING_ZONE"
			//"BR_MINIMAP",
			//"BLUE_ZONE"
		};

        class defs
        {
            class imageSets
            {
                files[]=
                {
                    "Vigrid-BattleRoyale/GUI/imagesets/dayzbr_gui.imageset"
                };
            };
            class gameLibScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Client/2_GameLib"
                };
            };
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Client/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Client/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Client/5_Mission"
                };
            };
        };
    };
};

class CfgVehicles
{
    class EffectArea;
    class BREffectArea: EffectArea {};
    class BlueZone: BREffectArea
    {
        scope=2;
    };
}
