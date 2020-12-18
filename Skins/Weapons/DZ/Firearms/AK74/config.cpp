class CfgPatches
{
	class DZBR_Weapons_Firearms_AK74
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms_AK74"
		};
	};
};
class cfgWeapons
{
	class AK74;
	class DZBR_AK74_Base: AK74
	{
		scope=0;
	};
	class DZBR_AK74_Bee: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\bee.paa"
		};
	};
	class DZBR_AK74_Rainbow: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\rainbow.paa"
		};
	};
	class DZBR_AK74_Apple: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\apple.paa"
		};
	};
	class DZBR_AK74_Glacier: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\glacier.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\glacier.rvmat"
		};
	};
	class DZBR_AK74_Depth: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\depth.paa"
		};
	};
	class DZBR_AK74_FB_Bug: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\bug.paa"
		};
	};
	class DZBR_AK74_FB_Reward_White_Black: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testwb.paa"
		};
	};
	class DZBR_AK74_FB_Reward_White_Orange: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testwo.paa"
		};
	};
	class DZBR_AK74_FB_Reward_White_Blue: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testwbl.paa"
		};
	};
	class DZBR_AK74_FB_Reward_White_Red: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testwr.paa"
		};
	};
	class DZBR_AK74_FB_Reward_White_Grey: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testwg.paa"
		};
	};
	class DZBR_AK74_FB_Reward_Black_Orange: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testbo.paa"
		};
	};
	class DZBR_AK74_FB_Reward_Black_Blue: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testbbl.paa"
		};
	};
	class DZBR_AK74_FB_Reward_Black_Red: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testbr.paa"
		};
	};
	class DZBR_AK74_FB_Reward_Black_Grey: DZBR_AK74_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK101\testbg.paa"
		};
	};

	//! -----------------------------------------------------

	class AKS74U;
	class DZBR_AKS74U_Base: AKS74U
	{
		scope=0;
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\nospray.rvmat"
		};
	};
	class DZBR_AKS74U_Bee: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\bee.paa"
		};
	};
	class DZBR_AKS74U_Rainbow: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\rainbow.paa"
		};
	};
	class DZBR_AKS74U_RattleSnake: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\rattlesnake.paa"
		};
	};
	class DZBR_AKS74U_Tactical: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\tactical.paa"
		};
	};
	class DZBR_AKS74U_FB_Bug: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\bug.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_White_Black: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testwb.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_White_Orange: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testwo.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_White_Blue: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testwbl.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_White_Red: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testwr.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_White_Grey: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testwg.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_Black_Orange: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testbo.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_Black_Blue: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testbbl.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_Black_Red: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testbr.paa"
		};
	};
	class DZBR_AKS74U_FB_Reward_Black_Grey: DZBR_AKS74U_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Firearms\AK74\testbg.paa"
		};
	};
};
