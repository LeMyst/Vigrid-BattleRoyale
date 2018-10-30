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
};

class CfgNonAIVehicles
{
	class ProxyAttachment;
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