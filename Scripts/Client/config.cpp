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
			//"BR_TRACE_ENABLED",
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
