class CfgPatches
{
	class DZBR_Weapons_Firearms_FAL
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms_FAL"
		};
	};
};
class cfgWeapons
{
	class FAL;
	class DZBR_FAL_Base: FAL
	{
		scope=0;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"DZ\weapons\firearms\fal\data\fal_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DZ\weapons\firearms\fal\data\fal.rvmat"
		};
	};
	class DZBR_FAL_Army: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\army.paa"
		};
	};
	class DZBR_FAL_Snow: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\snow.paa"
		};
	};
	class DZBR_FAL_BlueWeb: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\blueweb.paa"
		};
	};
	class DZBR_FAL_Vintage: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\vintage.paa"
		};
	};
	class DZBR_FAL_FB_Bug: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\bug.paa"
		};
	};
	class DZBR_FAL_FB_Reward_White_Black: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testwb.paa"
		};
	};
	class DZBR_FAL_FB_Reward_White_Orange: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testwo.paa"
		};
	};
	class DZBR_FAL_FB_Reward_White_Blue: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testwbl.paa"
		};
	};
	class DZBR_FAL_FB_Reward_White_Red: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testwr.paa"
		};
	};
	class DZBR_FAL_FB_Reward_White_Grey: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testwg.paa"
		};
	};
	class DZBR_FAL_FB_Reward_Black_Orange: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testbo.paa"
		};
	};
	class DZBR_FAL_FB_Reward_Black_Blue: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testbbl.paa"
		};
	};
	class DZBR_FAL_FB_Reward_Black_Red: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testbr.paa"
		};
	};
	class DZBR_FAL_FB_Reward_Black_Grey: DZBR_FAL_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\FAL\testbg.paa"
		};
	};
};
