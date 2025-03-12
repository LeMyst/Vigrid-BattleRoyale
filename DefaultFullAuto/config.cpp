class CfgPatches
{
    class vigrid_default_full_auto
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_default_full_auto
    {
        dir  = "DefaultFullAuto";
        name = "Vigrid Default Full Auto";
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
                    "Vigrid-BattleRoyale/DefaultFullAuto/Scripts/4_World"
                };
            };
        };
    };
};
