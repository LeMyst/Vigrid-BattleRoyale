class CfgPatches
{
    class vigrid_prevent_player_modifiers
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_prevent_player_modifiers
    {
        name = "Vigrid Prevent Player Modifiers";
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
                    "Vigrid-BattleRoyale/Extra/PreventPlayerModifiers/Scripts/4_World"
                };
            };
        };
    };
};
