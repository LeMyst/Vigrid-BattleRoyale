class CfgPatches
{
	class DZBR_Weapons_Pistols_M4
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Pistols_1911"
		};
	};
};
class cfgWeapons
{
	class Colt1911;
	class DZBR_Colt1911_Base: Colt1911
	{
		scope=0;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\pistols\1911\data\1911_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\pistols\1911\data\1911.rvmat"
		};
	};
	class DZBR_Colt1911_Tactical: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\tactical.paa"
		};
	};
	class DZBR_Colt1911_Shadows: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\shadows.paa"
		};
	};
	class DZBR_Colt1911_Blue: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\blue.paa"
		};
	};
	class DZBR_Colt1911_Orange: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\Orange.paa"
		};
	};
	class DZBR_Colt1911_Ukraine: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\ukraine.paa"
		};
	};
	class DZBR_Colt1911_FB_Bug: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\bug.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_White_Black: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testwb.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_White_Orange: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testwo.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_White_Blue: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testwbl.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_White_Red: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testwr.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_White_Grey: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testwg.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_Black_Orange: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testbo.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_Black_Blue: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testbbl.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_Black_Red: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testbr.paa"
		};
	};
	class DZBR_Colt1911_FB_Reward_Black_Grey: DZBR_Colt1911_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\1911\testbg.paa"
		};
	};
};
