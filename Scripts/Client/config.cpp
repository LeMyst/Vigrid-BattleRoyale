class CfgPatches
{
    class BattleRoyale_Scripts_Client
    {
        //--- DO NOT ADD "DayZExpansion_Scripts" BACK HERE. It was kept until 2026-08-22 to
        //--- order this mod's modded classes after Expansion's, on the belief that a
        //--- requiredAddons entry for an ABSENT addon is harmless. It is not: a dedicated
        //--- server without Expansion dies at addon load with the modal
        //---     Addon 'BattleRoyale_Scripts_Client' requires addon 'DayZExpansion_Scripts'
        //--- and an RPT that stops after the header - no addon list, no script log, nothing
        //--- to read. #267 only ever verified this against an OFFLINE CLIENT, and project.cfg
        //--- ships Expansion in AdditionalMPMods, so no Expansion-less server had ever booted
        //--- with the entry present. The ordering it bought is now bought by -mod= order
        //--- instead: list @Vigrid-BattleRoyale AFTER the Expansion mods (project.cfg does).
        requiredAddons[]=
        {
            "DZ_Data",
            "DZ_Scripts"
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
			//--- Note the missing trailing comma below: uncommenting MOVING_ZONE and the line after
			//--- it together is a rapify syntax error, not just a dead define.
			//"MOVING_ZONE"
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
