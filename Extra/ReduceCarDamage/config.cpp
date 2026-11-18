class CfgPatches
{
	class ReduceCarDamage
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={};
	};
};

class CfgMods
{
	class ReduceCarDamage
	{
		dir="ReduceCarDamage";
        picture="";
        action="";
        hideName=1;
        hidePicture=1;
        name="ReduceCarDamage";
        credits="DaemonForge";
        author="DaemonForge";
        authorID="0";
        version="0.1";
        extra=0;
        type="mod";
	    dependencies[]={ "World" };
	    class defs
	    {
	        class worldScriptModule
            {
                value="";
                // Changed for Vigrid-BattleRoyale (2026-11-18): was "ReduceCarDamage/scripts/4_world",
                // under the original PBO's prefix. Built from Extra/ReduceCarDamage, the prefix is
                // Vigrid-BattleRoyale\Extra\ReduceCarDamage.
                files[]={ "Vigrid-BattleRoyale/Extra/ReduceCarDamage/Scripts/4_World" };
            };
        };
    };
};
