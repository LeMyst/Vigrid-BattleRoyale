class CfgPatches
{
	class vigrid_prevent_weapon_raise
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
	class vigrid_prevent_weapon_raise
	{
		dir="PreventWeaponRaise";
		credits="Myst";
		type="mod";
		name="Vigrid-PreventWeaponRaise";

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
					"Vigrid-BattleRoyale/Extra/PreventWeaponRaise/Scripts/4_World"
				};
			};
		};
	};
};
