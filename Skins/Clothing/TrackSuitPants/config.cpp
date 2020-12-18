class CfgPatches
{
	class BattleRoyale_Skins_Clothing_TrackSuitPants
	{
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Characters_Pants"
		};
	};
};

class CfgVehicles
{
    class TrackSuitPants_ColorBase;
    class TrackSuitPants_DayZPodcast: TrackSuitPants_ColorBase
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"\BattleRoyale\Skins\Clothing\TrackSuitPants\dzbrpodcast_pants.paa",
			"\BattleRoyale\Skins\Clothing\TrackSuitPants\dzbrpodcast_pants.paa",
			"\BattleRoyale\Skins\Clothing\TrackSuitPants\dzbrpodcast_pants.paa"
		};
	};
};