class CfgPatches
{
	class DZBR_Weapons_Firearms_AKM
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms_AKM"
		};
	};
};
class cfgWeapons
{
	class AKM;
	class DZBR_AKM_Base: AKM
	{
		scope=0;
	};
	class DZBR_AKM_Mango: DZBR_AKM_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\akm_mango.paa"
		};
	};
	class DZBR_AKM_Apple: DZBR_AKM_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\apple.paa"
		};
	};
	class DZBR_AKM_Zerba: DZBR_AKM_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\zebra.paa"
		};
	};
	class DZBR_AKM_Exhibit: DZBR_AKM_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\exhibit.paa"
		};
	};
	class DZBR_AKM_Unnamed: DZBR_AKM_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\unnamed.paa"
		};
	};
	class DZBR_AKM_FB_Bug: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\bug.paa"
		};
	};
	class DZBR_AKM_FB_Reward_White_Black: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testwb.paa"
		};
	};
	class DZBR_AKM_FB_Reward_White_Orange: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testwo.paa"
		};
	};
	class DZBR_AKM_FB_Reward_White_Blue: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testwbl.paa"
		};
	};
	class DZBR_AKM_FB_Reward_White_Red: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testwr.paa"
		};
	};
	class DZBR_AKM_FB_Reward_White_Grey: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testwg.paa"
		};
	};
	class DZBR_AKM_FB_Reward_Black_Orange: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testbo.paa"
		};
	};
	class DZBR_AKM_FB_Reward_Black_Blue: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testbbl.paa"
		};
	};
	class DZBR_AKM_FB_Reward_Black_Red: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testbr.paa"
		};
	};
	class DZBR_AKM_FB_Reward_Black_Grey: AKM
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AKM\testbg.paa"
		};
	};
};
