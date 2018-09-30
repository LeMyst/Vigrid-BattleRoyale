class CfgPatches
{
	class dayzbr_scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]={};
	};
};

class CfgLoot
{
	class Weapons
	{
		num_items = 1;
		Gear_1[] = {"M4A1_Black","ACOGOptic","Mag_STANAG_30Rnd","Mag_STANAG_30Rnd"};
	};
	
	class Medical
	{
		num_items = 1;
		Gear_1[] = {"BandageDressing","Morphine","Splint"};
	};
	
	class Food
	{
		num_items = 1;
		Gear_1[] = {"TunaCan","SodaCan_Pipsi","Canteen","CanOpener"};
	};
	
	class Gear 
	{
		num_items = 1;
		Gear_1[] = {"DryBag_Black","BallisticHelmet_Black","HighCapacityVest_Black"};
	};
	
	
	class Center_Backpacks
	{
		num_items = 1;
		Gear_1[] = {"CarBattery"};
	};
};