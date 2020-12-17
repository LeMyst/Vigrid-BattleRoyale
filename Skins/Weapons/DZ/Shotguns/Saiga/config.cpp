class CfgPatches
{
	class DZBR_Weapons_Firearms_SaigaK
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms_SaigaK"
		};
	};
};
class cfgWeapons
{
	class Saiga;
	class DZBR_Saiga_Base: Saiga
	{
		scope=0;
		hiddenSelections[]=
		{
			"zbytek"
		};
		hiddenSelectionsTextures[]=
		{
			"DZ\weapons\shotguns\saiga\data\saiga_co.paa"
		};
	};
	class DZBR_Saiga_BlueWeb: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\blueweb.paa"
		};
	};
	class DZBR_Saiga_Neon: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\neon.paa"
		};
	};
	class DZBR_Saiga_Prestige: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\prestige.paa"
		};
	};
	class DZBR_Saiga_Silver: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\silver.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\silver.rvmat"
		};
	};
	class DZBR_Saiga_FB_Bug: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\bug.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_White_Black: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testwb.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_White_Orange: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testwo.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_White_Blue: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testwbl.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_White_Red: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testwr.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_White_Grey: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testwg.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_Black_Orange: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testbo.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_Black_Blue: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testbbl.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_Black_Red: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testbr.paa"
		};
	};
	class DZBR_Saiga_FB_Reward_Black_Grey: DZBR_Saiga_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"BattleRoyale\Skins\Weapons\DZ\Shotguns\Saiga\testbg.paa"
		};
	};
};
