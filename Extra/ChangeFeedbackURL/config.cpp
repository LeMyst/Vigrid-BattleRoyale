class CfgPatches
{
    class vigrid_change_feedback_url
    {
        requiredAddons[]=
        {
            "DZ_Scripts"
        };
    };
};

class CfgMods
{
    class vigrid_change_feedback_url
    {
        name = "Vigrid Change Feedback URL";
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
                    "Vigrid-BattleRoyale/Extra/ChangeFeedbackURL/Scripts/5_Mission"
                };
            };
        };
    };
};
