class CfgPatches
{
    class vigrid_vehicle3pp_default_fix
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_vehicle3pp_default_fix
    {
        name = "Vigrid Vehicle3PP Default Fix";
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
                    "Vigrid-BattleRoyale/Extra/Vehicle3PPDefaultFix/Scripts/4_World"
                };
            };
        };
    };
};
