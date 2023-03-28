class CfgPatches
{
    class BattleRoyale_Scripts
    {
        requiredAddons[]=
        {
            "DZ_Data",
            "DF_Scripts",
            "DayZExpansion_Scripts",
            "DZM_VPPAdminToolsScripts"
        };
    };
};

class CfgMods
{
    class DZ_BattleRoyale
    {
        dir = "DayZBR-Mod";
        credits = "Kegan - Modified by Myst";
        creditsJson = "DayZBR-Mod/Data/credits.json";
        type = "mod";
        inputs = "DayZBR-Mod/Data/Inputs.xml";
        name = "DayZ BattleRoyale";

        picture = "DayZBR-Mod/GUI/textures/Mod_Logo.paa";
        logo = "DayZBR-Mod/GUI/textures/Mod_Logo.paa";
        logoSmall = "DayZBR-Mod/GUI/textures/Mod_Logo.paa";
        logoOver = "DayZBR-Mod/GUI/textures/Mod_Logo.paa";

        dependencies[]=
        {
            "Game", "World", "Mission"
        };

        class defs
        {
            class imageSets
            {
                files[]=
                {
                    "DayZBR-Mod/GUI/imagesets/dayzbr_gui.imageset"
                };
            };
            /*class engineScriptModule
            {
                value = "";
                files[] =
                {
                    "DayZBR-Mod/Scripts/Common",
                    "DayZBR-Mod/Scripts/1_Core"
                };
            };*/
            class gameLibScriptModule
            {
                value = "";
                files[] =
                {
                    "DayZBR-Mod/Scripts/Common",
                    "DayZBR-Mod/Scripts/2_GameLib"
                };
            };
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "DayZBR-Mod/Scripts/Common",
                    "DayZBR-Mod/Scripts/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "DayZBR-Mod/Scripts/Common",
                    "DayZBR-Mod/Scripts/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "DayZBR-Mod/Scripts/Common",
                    "DayZBR-Mod/Scripts/5_Mission"
                };
            };
        };
    };
};

class CfgWorlds
{
    class DefaultWorld;
    class CAWorld: DefaultWorld
    {
        class Weather
        {
            class VolFog
            {
                CameraFog=0;
                Item1[]={0,0,0,0,0};
                Item2[]={0,0,0,0,0};
            };
        };
    };
};
	{
	};
