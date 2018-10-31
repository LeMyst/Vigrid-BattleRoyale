class CfgPatches
{
	class DayZBR_Assets
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};

class CfgVehicles {
	
	class HouseNoDestruct;
	class Land_Br_Wall: HouseNoDestruct
	{
		scope = 1;
		model = "\dayzbr\assets\p3ds\BR_Wall.p3d";
	};
	class Inventory_Base;
	class ItemOptics;
	class DBR_LongrangeOptic: ItemOptics
	{
		scope=2;
		displayName="$STR_cfgVehicles_LongrangeOptic0";
		descriptionShort="$STR_cfgVehicles_LongrangeOptic1";
		model="\DZ\weapons\attachments\optics\optic_longrange.p3d";
		animClass="Binoculars";
		rotationFlags=4;
		reversed=0;
		ContinuousActions[]={236};
		weight=700;
		itemSize[]={3,2};
		inventorySlot="weaponOpticsMosin";
		simulation="itemoptics";
		dispersionModifier=-0.00025000001;
		dispersionCondition="true";
		recoilModifier[]={1,1,1};
		memoryPointCamera="eyeScope_temp";
		cameraDir="cameraDir";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=100;
					healthLabels[]={1,0.69999999,0.5,0.30000001,0};
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_ca.paa",
								"DZ\weapons\attachments\data\mosin_scope.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_damaged_ca.paa",
								"DZ\weapons\attachments\data\mosin_scope_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_destroyed_ca.paa",
								"DZ\weapons\attachments\data\mosin_scope_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class OpticsInfo
		{
			useModelOptics=1;
			modelOptics="\DZ\weapons\attachments\optics\opticview_longrange.p3d";
			opticsDisablePeripherialVision=0.67000002;
			opticsFlare=1;
			opticsPPEffects[]=
			{
				"OpticsCHAbera3",
				"OpticsBlur1"
			};
			opticsZoomMin="0.3926/2.08";
			opticsZoomMax="0.3926/7.5";
			opticsZoomInit="0.3926/2.08";
			discretefov[]=
			{
				"0.3926/2.08",
				"0.3926/3.89",
				"0.3926/5.69",
				"0.3926/7.5"
			};
			discreteInitIndex=0;
			distanceZoomMin=100;
			distanceZoomMax=800;
			discreteDistance[]={100,200,300,400,500,600,700,800};
			discreteDistanceInitIndex=1;
			PPMaskProperties[]={0.5,0.5,0.40000001,0.050000001};
			PPLensProperties[]={1,0.15000001,0,0};
			PPBlurProperties=0.2;
		};
	};
	class DBR_HuntingOptic: ItemOptics
	{
		scope=2;
		displayName="$STR_cfgVehicles_HuntingOptic0";
		descriptionShort="$STR_cfgVehicles_HuntingOptic1";
		model="\DZ\weapons\attachments\optics\optic_hunting.p3d";
		animClass="Binoculars";
		rotationFlags=4;
		reversed=0;
		ContinuousActions[]={236};
		weight=700;
		itemSize[]={3,2};
		inventorySlot="weaponOpticsMosin";
		simulation="itemoptics";
		dispersionModifier=-0.00025000001;
		dispersionCondition="true";
		recoilModifier[]={1,1,1};
		memoryPointCamera="eyeScope_temp";
		cameraDir="cameraDir";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=100;
					healthLabels[]={1,0.69999999,0.5,0.30000001,0};
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_ca.paa",
								"DZ\weapons\attachments\data\terra.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_damaged_ca.paa",
								"DZ\weapons\attachments\data\terra_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_destroyed_ca.paa",
								"DZ\weapons\attachments\data\terra_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class OpticsInfo
		{
			useModelOptics=1;
			modelOptics="\DZ\weapons\attachments\optics\opticview_longrange.p3d";
			opticsDisablePeripherialVision=0.67000002;
			opticsFlare=1;
			opticsPPEffects[]=
			{
				"OpticsCHAbera3",
				"OpticsBlur1"
			};
			opticsZoomMin="0.3926/4";
			opticsZoomMax="0.3926/12";
			opticsZoomInit="0.3926/4";
			discretefov[]=
			{
				"0.3926/4",
				"0.3926/6",
				"0.3926/8",
				"0.3926/10",
				"0.3926/12"
			};
			discreteInitIndex=0;
			distanceZoomMin=100;
			distanceZoomMax=1000;
			discreteDistance[]={100,200,300,400,500,600,700,800,1000};
			discreteDistanceInitIndex=1;
			PPMaskProperties[]={0.5,0.5,0.40000001,0.050000001};
			PPLensProperties[]={1,0.15000001,0,0};
			PPBlurProperties=0.2;
		};
	};

	class Edible_Base;
	class Bottle_Base;
	class SodaCan_ColorBase: Edible_Base
	{
		model="\dz\gear\drinks\SodaCan.p3d";
		stackedRandom=0;
		autoQuickbar=1;
		itemSize[]={1,2};
		SingleUseActions[]={507,505,544};
		ContinuousActions[]={108,215};
		InteractActions[]={};
		weight=33;
		stackedUnit="";
		varQuantityInit=330;
		varQuantityMin=0;
		varQuantityMax=330;
		isMeleeWeapon=1;
		hiddenSelections[]=
		{
			"camoGround"
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=100;
					healthLabels[]={1,0.69999999,0.5,0.30000001,0};
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\gear\drinks\data\Drink_WaterPouch_Natural.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\gear\drinks\data\Drink_WaterPouch_Natural_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\gear\drinks\data\Drink_WaterPouch_Natural_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class Nutrition
		{
			totalVolume=1;
			energy=43.5;
			water=89;
			nutritionalIndex=1;
			toxicity=0;
		};
		class MeleeModes
		{
			class Default
			{
				ammo="MeleeLightBlunt";
				range=1;
			};
			class Heavy
			{
				ammo="MeleeLightBlunt_Heavy";
				range=1;
			};
			class Sprint
			{
				ammo="MeleeLightBlunt_Heavy";
				range=2.8;
			};
		};
	};
	class DBR_SodaCan_Monsta_Stam: SodaCan_ColorBase
	{
		scope=2;
		displayName="Stamina Monsta Drink";
		descriptionShort="Drink this and get x amount of run distance.";
		varQuantityInit=100;
		varQuantityMin=0;
		varQuantityMax=100;
		hiddenSelectionsTextures[]=
		{
			"assets\textures\drinks\SodaCan_monsta.paa",
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class Drinking_loop
				{
					soundSet="Drinking_loop_SoundSet";
					id=200;
				};
				class SodaCan_in_A
				{
					soundSet="SodaCan_in_A_SoundSet";
					id=201;
				};
				class SodaCan_in_B
				{
					soundSet="SodaCan_in_B_SoundSet";
					id=202;
				};
				class SodaCan_in_C
				{
					soundSet="SodaCan_in_C_SoundSet";
					id=203;
				};
				class SodaCan
				{
					soundSet="SodaCan_SoundSet";
					id=204;
				};
				class SodaCan_out_A
				{
					soundSet="SodaCan_out_A_SoundSet";
					id=205;
				};
				class SodaCan_out_B
				{
					soundSet="SodaCan_out_B_SoundSet";
					id=206;
				};
				class SodaCan_out_C
				{
					soundSet="SodaCan_out_C_SoundSet";
					id=207;
				};
				class WaterBottle_Whoosh
				{
					soundSet="WaterBottle_Whoosh_SoundSet";
					id=16;
				};
				class WaterBottle_WhooshShort
				{
					soundSet="WaterBottle_WhooshShort_SoundSet";
					id=17;
				};
				class WaterBottle_WhooshHeavy
				{
					soundSet="WaterBottle_WhooshHeavy_SoundSet";
					id=18;
				};
			};
		};
	};
	class DBR_SodaCan_Monsta_Heal: SodaCan_ColorBase
	{
		scope=2;
		displayName="Healing Monsta Drink";
		descriptionShort="Drink this and get x amount of regen.";
		SingleUseActions[]={}; //no single use actions
		ContinuousActions[]={10000};
		varQuantityInit=100;
		varQuantityMin=0;
		varQuantityMax=100;
		hiddenSelectionsTextures[]=
		{
			"assets\textures\drinks\SodaCan_monsta_heal.paa",
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class Drinking_loop
				{
					soundSet="Drinking_loop_SoundSet";
					id=200;
				};
				class SodaCan_in_A
				{
					soundSet="SodaCan_in_A_SoundSet";
					id=201;
				};
				class SodaCan_in_B
				{
					soundSet="SodaCan_in_B_SoundSet";
					id=202;
				};
				class SodaCan_in_C
				{
					soundSet="SodaCan_in_C_SoundSet";
					id=203;
				};
				class SodaCan
				{
					soundSet="SodaCan_SoundSet";
					id=204;
				};
				class SodaCan_out_A
				{
					soundSet="SodaCan_out_A_SoundSet";
					id=205;
				};
				class SodaCan_out_B
				{
					soundSet="SodaCan_out_B_SoundSet";
					id=206;
				};
				class SodaCan_out_C
				{
					soundSet="SodaCan_out_C_SoundSet";
					id=207;
				};
				class WaterBottle_Whoosh
				{
					soundSet="WaterBottle_Whoosh_SoundSet";
					id=16;
				};
				class WaterBottle_WhooshShort
				{
					soundSet="WaterBottle_WhooshShort_SoundSet";
					id=17;
				};
				class WaterBottle_WhooshHeavy
				{
					soundSet="WaterBottle_WhooshHeavy_SoundSet";
					id=18;
				};
			};
		};
	};
};

class CfgNonAIVehicles
{
	class ProxyAttachment;
	class ProxyOptic_PUScope: ProxyAttachment
	{
		scope=2;
		inventorySlot="weaponOpticsMosin";
		model="\DZ\weapons\attachments\optics\optic_puscope.p3d";
	};
	class ProxyOptic_LongRange: ProxyAttachment
	{
		scope=2;
		inventorySlot="weaponOpticsMosin";
		model="\DZ\weapons\attachments\optics\optic_longrange.p3d";
	};
	class ProxyOptic_Hunting: ProxyAttachment
	{
		scope=2;
		inventorySlot="weaponOpticsMosin";
		model="\DZ\weapons\attachments\optics\optic_hunting.p3d";
	};
};
