class CfgPatches
{
	class DZBR_Weapons_Shotguns_MP133
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Shotguns_MP133"
		};
	};
};
class cfgWeapons
{
	class Mp133Shotgun;
	class DZBR_MP133_Base: Mp133Shotgun
	{
		scope=0;
	};
	class DZBR_MP133_Battle: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\battle.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\battle.rvmat"
		};
	};
	class DZBR_MP133_Hotline: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\hotline.paa"
		};
	};
	class DZBR_MP133_Taser: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\taser.paa"
		};
	};
	class DZBR_MP133_Rainbow: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\rainbow.paa"
		};
	};
	class DZBR_MP133_FB_Bug: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\bug.paa"
		};
	};
	class DZBR_MP133_FB_Reward_White_Black: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testwb.paa"
		};
	};
	class DZBR_MP133_FB_Reward_White_Orange: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testwo.paa"
		};
	};
	class DZBR_MP133_FB_Reward_White_Blue: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testwbl.paa"
		};
	};
	class DZBR_MP133_FB_Reward_White_Red: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testwr.paa"
		};
	};
	class DZBR_MP133_FB_Reward_White_Grey: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testwg.paa"
		};
	};
	class DZBR_MP133_FB_Reward_Black_Orange: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testbo.paa"
		};
	};
	class DZBR_MP133_FB_Reward_Black_Blue: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testbbl.paa"
		};
	};
	class DZBR_MP133_FB_Reward_Black_Red: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testbr.paa"
		};
	};
	class DZBR_MP133_FB_Reward_Black_Grey: DZBR_MP133_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\MP133\testbg.paa"
		};
	};
};
