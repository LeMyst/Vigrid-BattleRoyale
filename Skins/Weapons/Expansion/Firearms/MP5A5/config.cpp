class CfgPatches
{
	class DZBR_DayZExpansion_Weapons_SMG_MP5
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DayZExpansion_Weapons_SMG_MP5"
		};
	};
};
class cfgWeapons
{
	class Expansion_MP5;
	class DZBR_Expansion_MP5_Base: Expansion_MP5
	{
		scope=0;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"dayzexpansion\objects\weapons\firearms\smg\mp5\data\dayz_co.paa"
		};
	};
	class DZBR_Expansion_MP5_Abyss: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\abyss.paa"
		};
	};
	class DZBR_Expansion_MP5_Blue: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\blue.paa"
		};
	};
	class DZBR_Expansion_MP5_Exhibit: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\exhibit.paa"
		};
	};
	class DZBR_Expansion_MP5_Orange: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\orange.paa"
		};
	};
	class DZBR_Expansion_MP5_Killer: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\killer.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Bug: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\bug.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_White_Black: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testwb.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_White_Orange: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testwo.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_White_Blue: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testwbl.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_White_Red: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testwr.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_White_Grey: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testwg.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_Black_Orange: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testbo.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_Black_Blue: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testbbl.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_Black_Red: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testbr.paa"
		};
	};
	class DZBR_Expansion_MP5_FB_Reward_Black_Grey: DZBR_Expansion_MP5_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\Expansion\Firearms\MP5A5\testbg.paa"
		};
	};
};
