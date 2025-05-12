class CfgPatches
{
	class vigrid_limit_unconscious_time
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts"
		};
	};
};

class CfgMods
{
	class vigrid_limit_unconscious_time
	{
		dir="LimitUnconsciousTime";
		credits="Myst";
		type="mod";
		name="Vigrid-LimitUnconsciousTime";

		dependencies[]=
		{
			"World"
		};

		class defs
		{
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"Vigrid-BattleRoyale/Extra/LimitUnconsciousTime/Scripts/4_World"
				};
			};
		};
	};
};
