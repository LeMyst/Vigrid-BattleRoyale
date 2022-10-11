class CfgPatches
{
	class BattleRoyale_Scripts
	{
		requiredAddons[]=
		{
			"DZ_Data",
			"DayZExpansion_Scripts"
		};
	};
};

class CfgMods
{
	class DZ_BattleRoyale
	{
		dir = "DayZBR-Mod";
		credits = "Kegan - Modified by Myst";
		creditsJson = "DayZBR-Mod/Data/credits.json";
		type = "mod";
		name = "DayZ BattleRoyale";
		
		//picture = "BattleRoyale/GUI/textures/logo.edds";
		//logo = "BattleRoyale/GUI/textures/logo.edds";
		//logoSmall = "BattleRoyale/GUI/textures/logo.edds";
		//logoOver = "BattleRoyale/GUI/textures/logo.edds";
		
		dependencies[]= 
		{
			"Game",
			"World",
			"Mission"
		};
		
		class defs
		{
			class imageSets
			{
				files[]=
				{
					"DayZBR-Mod/GUI/imagesets/dayzbr_gui.imageset"
				};
			};
			class engineScriptModule
			{
				value = "";
				files[] =
				{
					"DayZBR-Mod/Scripts/1_Core"
				};
			};
			class gameLibScriptModule
			{
				value = "";
				files[] =
				{
					"DayZBR-Mod/Scripts/2_GameLib"
				};
			};
			class gameScriptModule
			{
				value = "";
				files[] =
				{
					"DayZBR-Mod/Scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value = "";
				files[] =
				{
					"DayZBR-Mod/Scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] =
				{
					"DayZBR-Mod/Scripts/5_Mission"
				};
			};
		};
	};
};
