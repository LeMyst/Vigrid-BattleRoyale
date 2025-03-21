class CfgPatches
{
    class vigrid_spawn_with_battery
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_spawn_with_battery
    {
        name = "Vigrid Spawn With Battery";
        type = "mod";
        dependencies[]=
        {
            "World"
        };

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/SpawnWithBattery/Scripts/4_World"
                };
            };
        };
    };
};
