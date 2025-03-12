class CfgPatches
{
    class BattleRoyale_Scripts_Server
    {
        requiredAddons[]=
        {
            "DZ_Data",
            "DZ_Scripts",
            "BattleRoyale_Scripts_Client"
        };
    };
};

class CfgMods
{
    class DZ_BattleRoyale_Server
    {
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
            class gameScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Server/3_Game"
                };
            };
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Server/4_World"
                };
            };
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Scripts/Server/5_Mission"
                };
            };
        };
    };
};
