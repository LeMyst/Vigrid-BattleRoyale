class CfgPatches
{
    class BattleRoyale_Scripts_Server
    {
        requiredAddons[]=
        {
            "DZ_Data",
            "DF_Scripts",
            "BattleRoyale_Scripts_Client"
        };
    };
};

class CfgMods
{
    class DZ_BattleRoyale_Server
    {
        dir  = "Vigrid-BattleRoyale";
        name = "DayZ BattleRoyale SERVER";
        type = "mod";
        dependencies[]=
        {
            "Game",
            "World",
            "Mission"
        };

        class defs
        {
            class gameLibScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts_Server/2_GameLib"
                };
            };
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts_Server/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts_Server/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts_Server/5_Mission"
                };
            };
        };
    };
};
