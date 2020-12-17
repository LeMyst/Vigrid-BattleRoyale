class CfgPatches
{
	class BattleRoyale_Skins_Clothing_TrackSuitJacket
	{
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Characters_Tops"
		};
	};
};

class CfgVehicles
{
    class TrackSuitJacket_ColorBase;
    class TrackSuitJacket_DayZPodcast: TrackSuitJacket_ColorBase
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"\BattleRoyale\Skins\Clothing\TrackSuitJacket\dzbrpodcast_jacket.paa",
			"\BattleRoyale\Skins\Clothing\TrackSuitJacket\dzbrpodcast_jacket.paa",
			"\BattleRoyale\Skins\Clothing\TrackSuitJacket\dzbrpodcast_jacket.paa"
		};
	};
};