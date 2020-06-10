class CfgPatches
{
	class BattleRoyale_Scripts
	{
		requiredAddons[]=
		{
			"DayZExpansion_Scripts" 
		};
	};
};
class CfgMods
{
	class DZ_BattleRoyale
	{
		dir = "BattleRoyale";		
		credits = "Kegan";
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
					//"BattleRoyale/GUI/imagesets/some_image_set.imageset"
				};
			};
			class engineScriptModule
			{
				value = "";
				files[] =
				{
					"BattleRoyale/Scripts/1_Core"
				};
			};
			class gameLibScriptModule
			{
				value = "";
				files[] =
				{
					"BattleRoyale/Scripts/2_GameLib"
				};
			};
			class gameScriptModule
			{
				value = "";
				files[] =
				{
					"BattleRoyale/Scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value = "";
				files[] =
				{
					"BattleRoyale/Scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value = "";
				files[] =
				{
					"BattleRoyale/Scripts/5_Mission"
				};
			};
		};
	};
};