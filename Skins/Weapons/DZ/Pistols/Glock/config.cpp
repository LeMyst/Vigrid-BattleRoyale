class CfgPatches
{
	class DZBR_Weapons_Pistols_Glock
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Pistols_Glock"
		};
	};
};
class cfgWeapons
{
	class Glock19;
	class DZBR_Glock19_Base: Glock19
	{
		scope=0;
		hiddenSelections[]=
		{
			"zbytek"
		};
		hiddenSelectionsTextures[]=
		{
			"DZ\weapons\pistols\glock\data\glock19_co.paa"
		};
	};
	class DZBR_Glock19_Tactical: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\tactical.paa"
		};
	};
	class DZBR_Glock19_PinkCamo: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\pinkcamo.paa"
		};
	};
	class DZBR_Glock19_Sandstorm: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\sandstorm.paa"
		};
	};
	class DZBR_Glock19_White: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\white.paa"
		};
	};
	class DZBR_Glock19_FB_Bug: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\bug.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_White_Black: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testwb.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_White_Orange: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testwo.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_White_Blue: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testwbl.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_White_Red: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testwr.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_White_Grey: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testwg.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_Black_Orange: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testbo.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_Black_Blue: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testbbl.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_Black_Red: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testbr.paa"
		};
	};
	class DZBR_Glock19_FB_Reward_Black_Grey: DZBR_Glock19_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\Glock\testbg.paa"
		};
	};
};
