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
		num_items = 23;
		Gear_1[] = {"M4A1_Black","ACOGOptic","Mag_STANAG_30Rnd","Mag_STANAG_30Rnd"};
		Gear_2[] = {"M4A1","ACOGOptic","Mag_STANAG_30Rnd"};
		Gear_3[] = {"M4A1","Mag_STANAG_30Rnd"};
		Gear_4[] = {"M4A1","M68Optic","Mag_STANAG_30Rnd","Battery9V"};
		Gear_5[] = {"M4A1","M68Optic","Mag_STANAG_30Rnd","Battery9V"};
		Gear_6[] = {"AKM","PSO1Optic","Mag_AKM_30Rnd"}; 
		Gear_7[] = {"AKM","PSO11Optic","Mag_AKM_30Rnd"};
		Gear_8[] = {"AKM","Mag_AKM_30Rnd"};
		Gear_9[] = {"AKM","KashtanOptic","Mag_AKM_30Rnd"};
		Gear_10[] = {"Mosin9130","Ammo_762x54"};
		Gear_11[] = {"Mosin9130","PUScopeOptic","Ammo_762x54"};
		Gear_12[] = {"FirefighterAxe","UMP45","Mag_UMP_25Rnd"};
		Gear_13[] = {"MP5K","Mag_MP5_15Rnd","Mag_MP5_30Rnd"};
		Gear_14[] = {"SVD","PSO11Optic","Mag_SVD_10Rnd"};
		Gear_15[] = {"FNX45","Mag_FNX45_15Rnd","Mag_FNX45_15Rnd"};
		Gear_16[] = {"M4A1","M4_Suppressor","Mag_STANAGCoupled_30Rnd"};
		Gear_17[] = {"Mag_AKM_Drum75Rnd","AmmoBox_762x39_20Rnd","Ammo_556x45"};
		Gear_18[] = {"AmmoBox_762x54_20Rnd","Mag_AKM_30Rnd","Ammo_9x19"};
		Gear_19[] = {"AmmoBox_762x39_20Rnd","Mag_AKM_30Rnd","Ammo_9x19"};
		Gear_20[] = {"Mag_UMP_25Rnd","Ammo_45ACP","Ammo_45ACP"};
		Gear_21[] = {"Mag_STANAGCoupled_30Rnd","Ammo_45ACP","Mag_MP5_30Rnd"};
		Gear_22[] = {"Mag_STANAG_30Rnd","Ammo_45ACP","Mag_MP5_30Rnd"};
		Gear_23[] = {"Mag_STANAG_30Rnd","Mag_MP5_30Rnd","Ammo_556x45"};
	};
	
	class Medical
	{
		num_items = 9;
		Gear_1[] = {"BandageDressing","Morphine","Splint"};
		Gear_2[] = {"Morphine","Rag","FirstAidKit"};
		Gear_3[] = {"Morphine","Rag","FirstAidKit"};
		Gear_4[] = {"SalineBagIV","Rag","FirstAidKit"};
		Gear_5[] = {"SalineBagIV","Rag","FirstAidKit"};
		Gear_6[] = {"BandageDressing","Morphine"};
		Gear_7[] = {"BandageDressing","Splint"};
		Gear_8[] = {"BandageDressing","Splint"};
		Gear_9[] = {"BandageDressing","Morphine"};
	};
	
	class Food
	{
		num_items = 10;
		Gear_1[] = {"TunaCan","SodaCan_Pipsi","Canteen","CanOpener"};
		Gear_2[] = {"TacticalBaconCan","HuntingKnife","SodaCan_Cola"};
		Gear_3[] = {"TacticalBaconCan","WaterBottle","SodaCan_Kvass"};
		Gear_4[] = {"TacticalBaconCan","HuntingKnife","SodaCan_Kvass"};
		Gear_5[] = {"PeachesCan","WaterBottle"};
		Gear_6[] = {"PeachesCan","Apple","SodaCan_Kvass"};
		Gear_7[] = {"PeachesCan","Canteen","HighCapacityVest_Black"};
		Gear_8[] = {"PeachesCan","HuntingKnife","SodaCan_Kvass"};
		Gear_9[] = {"Apple","Canteen","kiwi"};
		Gear_10[] = {"Apple","HuntingKnife","SodaCan_Kvass"};
	};
	
	class Gear 
	{
		num_items = 17;
		Gear_1[] = {"DryBag_Black","BallisticHelmet_Black","HighCapacityVest_Black"};
		Gear_2[] = {"GorkaPants_Summer","GorkaEJacket_Summer","HighCapacityVest_Black"};
		Gear_3[] = {"GorkaEJacket_Flat","GorkaPants_Flat","HighCapacityVest_Black"};
		Gear_4[] = {"GorkaPants_PautRev","GorkaEJacket_PautRev","HighCapacityVest_Black","CarBattery"};
		Gear_5[] = {"USMCPants_Desert","USMCJacket_Desert","Ammo_556x45"};
		Gear_6[] = {"USMCPants_Woodland","USMCJacket_Woodland","Ammo_556x45"};
		Gear_7[] = {"MedicalScrubsPants_Blue","MedicalScrubsPants_Blue","MedicalScrubsHat_Blue"};
		Gear_8[] = {"MedicalScrubsPants_Green","MedicalScrubsPants_Green","MedicalScrubsHat_Green"};
		Gear_9[] = {"MedicalScrubsPants_White","MedicalScrubsPants_White","MedicalScrubsHat_White","CarBattery"};
		Gear_10[] = {"GorkaHelmetComplete","AmmoBox_762x39_20Rnd","Mag_AKM_30Rnd"};
		Gear_11[] = {"MotoHelmet_Black","AmmoBox_762x39_20Rnd","HighCapacityVest_Black"};
		Gear_12[] = {"MotoHelmet_Lime","AmmoBox_762x39_20Rnd","Ammo_556x45"};
		Gear_13[] = {"BallisticHelmet_Black","HighCapacityVest_Black","Ammo_556x45"};
		Gear_14[] = {"BallisticHelmet_UN","HighCapacityVest_Black","Ammo_556x45"};
		Gear_15[] = {"GorkaHelmetComplete","M65Jacket_Olive","Ammo_556x45"};
		Gear_16[] = {"BallisticHelmet_Green","AmmoBox_762x39_20Rnd","AssaultBag_Black"};
		Gear_17[] = {"GorkaHelmetComplete","M65Jacket_Black","ImprovisedBag","CarBattery"};
	};
	
	
	class Center_Backpacks
	{
		num_items = 1;
		Gear_1[] = {"PeachesCan_Opened","PeachesCan_Opened","FNX45","Mag_FNX45_15Rnd","Mag_FNX45_15Rnd","WaterBottle","CarBattery"};
		Gear_2[] = {"AK_Suppressor","PeachesCan_Opened","FNX45","Mag_FNX45_15Rnd","Mag_FNX45_15Rnd","WaterBottle","CarBattery"};
		Gear_3[] = {"M4_Suppressor","PeachesCan_Opened","FNX45","Mag_FNX45_15Rnd","Mag_FNX45_15Rnd","WaterBottle","CarBattery"};
	};
};