class CfgPatches
{
    class vigrid_spawn_weapon_chambered
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_spawn_weapon_chambered
    {
        name = "Vigrid Spawn Weapon Chambered";
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
                    "Vigrid-BattleRoyale/Extra/SpawnWeaponChambered/Scripts/4_World"
                };
            };
        };
    };
};
