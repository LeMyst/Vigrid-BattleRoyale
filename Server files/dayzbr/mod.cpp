class CfgMods
{
	class dayzbr
	{
		dir = "dayzbr";
		picture = "";
		action = "";
		hideName = 1;
		hidePicture = 1;
		name = "DayZ BattleRoyale";
		credits = "Kegan, Vulkan, BigBen";
		author = "Kegan";
		authorID = "0"; 
		version = "1.0"; 
		extra = 0;
		type = "mod";
		
		dependencies[] = {"Game", "World", "Mission"};
		
		class defs
		{
			class gameScriptModule
			{
				value = "";
				files[] = {"dayzbr/scripts/scripts/3_Game"};
			};

			class worldScriptModule
			{
				value = "";
				files[] = {"dayzbr/scripts/scripts/4_World"};
			};

			class missionScriptModule
			{
				value = "";
				files[] = {"dayzbr/scripts/scripts/5_Mission"};
			};
		}
	};
};
