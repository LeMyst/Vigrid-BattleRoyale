class CfgPatches
{
    class vigrid_change_party_marker_color
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_change_party_marker_color
    {
        name = "Vigrid Change Party Marker Color";
        type = "mod";
        dependencies[]=
        {
            "Mission"
        };

        class defs
        {
            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "Vigrid-BattleRoyale/Extra/ChangePartyMarkerColor/Scripts/5_Mission"
                };
            };
        };
    };
};
