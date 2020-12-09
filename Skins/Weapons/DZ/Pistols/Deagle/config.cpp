class CfgPatches
{
	class DZBR_Weapons_Pistols_DE
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Pistols_DE"
		};
	};
};
class cfgWeapons
{
	class Deagle;
	class DZBR_Deagle_Base: Deagle
	{
		scope=0;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\pistols\DE\data\deagle_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\deagle_mat.rvmat"
		};
	};
	class DZBR_Deagle_BlueToy: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\bluetoy.paa"
		};
	};
	class DZBR_Deagle_Exhibit: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\exhibit.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\deagle.rvmat"
		};
	};
	class DZBR_Deagle_PinkToy: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\pinktoy.paa"
		};
	};
	class DZBR_Deagle_RedLine: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\redline.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\deagle.rvmat"
		};
	};
	class DZBR_Deagle_Ice: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\ice.paa"
		};
	};
	class DZBR_Deagle_FB_Bug: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\bug.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_White_Black: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testwb.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_White_Orange: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testwo.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_White_Blue: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testwbl.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_White_Red: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testwr.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_White_Grey: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testwg.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_Black_Orange: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testbo.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_Black_Blue: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testbbl.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_Black_Red: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testbr.paa"
		};
	};
	class DZBR_Deagle_FB_Reward_Black_Grey: DZBR_Deagle_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Deagle\testbg.paa"
		};
	};
};
