class CfgPatches
{
    class vigrid_prevent_broken_legs
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_prevent_broken_legs
    {
        name = "Vigrid Prevent Broken Legs";
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
                    "Vigrid-BattleRoyale/Extra/PreventBrokenLegs/Scripts/4_World"
                };
            };
        };
    };
};
