class CfgPatches
{
	class DZBR_Weapons_Firearms_Winchester70
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms_Winchester70"
		};
	};
};
class cfgWeapons
{
	class Winchester70;
	class DZBR_Winchester70_Base_NoCurves: Winchester70
	{
		scope=0;
		hiddenSelectionsMaterials[]=
		{
			"DZ\weapons\firearms\Winchester70\data\nocurves.rvmat"
		};
	};
	class DZBR_Winchester70_Depth: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\depth.paa"
		};
	};
	class DZBR_Winchester70_Digital: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\digital.paa"
		};
	};
	class DZBR_Winchester70_Love: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\love.paa"
		};
	};
	class DZBR_Winchester70_OldTimes: Winchester70
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\oldtimes.paa"
		};
	};
	class DZBR_Winchester70_FB_Bug: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\bug.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_White_Black: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testwb.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_White_Orange: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testwo.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_White_Blue: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testwbl.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_White_Red: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testwr.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_White_Grey: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testwg.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_Black_Orange: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testbo.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_Black_Blue: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testbbl.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_Black_Red: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testbr.paa"
		};
	};
	class DZBR_Winchester70_FB_Reward_Black_Grey: DZBR_Winchester70_Base_NoCurves
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\Winchester70\testbg.paa"
		};
	};
};
