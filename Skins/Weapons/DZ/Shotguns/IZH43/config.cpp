class CfgPatches
{
	class DZBR_Weapons_Shotguns_Izh43
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Shotguns_Izh43"
		};
	};
};
class cfgWeapons
{
	class Izh43Shotgun;
	class DZBR_Izh43Shotgun_Base: Izh43Shotgun
	{
		scope=0;
		hiddenSelections[]=
		{
			"zbytek"
		};
		hiddenSelectionsTextures[]=
		{
			"DZ\weapons\shotguns\izh43\data\izh43_co.paa"
		};
	};
	class DZBR_Izh43Shotgun_Black: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\black.paa"
		};
	};
	class DZBR_Izh43Shotgun_GoldenPrestige: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\goldenprestige.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\gold.rvmat"
		};
	};
	class DZBR_Izh43Shotgun_SilverPrestige: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\silverprestige.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\silver.rvmat"
		};
	};
	class DZBR_Izh43Shotgun_Zebra: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\zebra.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Bug: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\bug.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_White_Black: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwb.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_White_Orange: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwo.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_White_Blue: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwbl.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_White_Red: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwr.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_White_Grey: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwg.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_Black_Orange: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbo.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_Black_Blue: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbbl.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_Black_Red: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbr.paa"
		};
	};
	class DZBR_Izh43Shotgun_FB_Reward_Black_Grey: DZBR_Izh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbg.paa"
		};
	};

	//! --------------------------------------------------------------

	class SawedoffIzh43Shotgun;
	class DZBR_SawedoffIzh43Shotgun_Base: SawedoffIzh43Shotgun
	{
		scope=0;
		hiddenSelections[]=
		{
			"zbytek"
		};
		hiddenSelectionsTextures[]=
		{
			"DZ\weapons\shotguns\izh43\data\izh43_co.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_Black: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\black.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_GoldenPrestige: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\goldenprestige.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\gold.rvmat"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_SilverPrestige: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\silverprestige.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\silver.rvmat"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_Zebra: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\zebra.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Bug: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\bug.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Black: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwb.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Orange: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwo.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Blue: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwbl.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Red: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwr.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_White_Grey: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testwg.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Orange: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbo.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Blue: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbbl.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Red: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbr.paa"
		};
	};
	class DZBR_SawedoffIzh43Shotgun_FB_Reward_Black_Grey: DZBR_SawedoffIzh43Shotgun_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\IZH43\testbg.paa"
		};
	};
};
