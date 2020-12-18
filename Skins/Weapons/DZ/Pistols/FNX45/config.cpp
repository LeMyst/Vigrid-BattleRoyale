class CfgPatches
{
	class DZBR_Weapons_Pistols_FNX45
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Pistols_FNX45"
		};
	};
};
class cfgWeapons
{
	class FNX45;
	class DZBR_FNX45_Base: FNX45
	{
		scope=0;
		hiddenSelections[]=
		{
			"zbytek"
		};
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\pistols\fnx45\data\herstal45tactical_co.paa"
		};
	};
	class DZBR_FNX45_Tactical: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\tactical.paa"
		};
	};
	class DZBR_FNX45_Exhibit: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\exhibit.paa"
		};
	};
	class DZBR_FNX45_Desert: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\desert.paa"
		};
	};
	class DZBR_FNX45_Ukraine: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\ukraine.paa"
		};
	};
	class DZBR_FNX45_FB_Bug: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\bug.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_White_Black: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testwb.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_White_Orange: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testwo.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_White_Blue: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testwbl.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_White_Red: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testwr.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_White_Grey: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testwg.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_Black_Orange: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testbo.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_Black_Blue: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testbbl.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_Black_Red: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testbr.paa"
		};
	};
	class DZBR_FNX45_FB_Reward_Black_Grey: DZBR_FNX45_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Pistols\FNX45\testbg.paa"
		};
	};
};
