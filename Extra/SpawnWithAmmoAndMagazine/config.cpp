class CfgPatches
{
    class vigrid_spawn_with_ammo_and_magazine
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_spawn_with_ammo_and_magazine
    {
        name = "Vigrid Spawn With Ammo And Magazine";
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
                    "Vigrid-BattleRoyale/Extra/SpawnWithAmmoAndMagazine/Scripts/4_World"
                };
            };
        };
    };
};
