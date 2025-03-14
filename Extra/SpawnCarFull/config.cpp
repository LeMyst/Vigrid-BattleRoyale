class CfgPatches
{
    class vigrid_spawn_car_full
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_spawn_car_full
    {
        name = "Vigrid Spawn Car Full";
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
                    "Vigrid-BattleRoyale/Extra/SpawnCarFull/Scripts/4_World"
                };
            };
        };
    };
};
