class CfgPatches
{
	class DZ_Weapons_Firearms_AKM
	{
		units[]=
		{
			"AKM"
		};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data",
			"DZ_Weapons_Firearms"
		};
	};
};
class Mode_Safe;
class Mode_SemiAuto;
class Mode_Burst;
class Mode_FullAuto;
class cfgWeapons
{
	class AKM;
	class GP_Fal_Base: AKM
	{
		scope=0;
		weight=3700;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		discreteDistance[]={100,200,300,400,500,600,700,800,900,1000};
		discreteDistanceInitIndex=0;
		modelOptics="-";
		distanceZoomMin=100;
		distanceZoomMax=100;
		PPDOFProperties[]={1,0.60000002,200000,0.30000001,3,0.1};
		optics=1;
		opticsFlare=0;
		ironsightsExcludingOptics[]=
		{
			"BUISOptic",
			"M68Optic",
			"M4_T3NRDSOptic",
			"ReflexOptic",
			"ACOGOptic"
		};
		value=0;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_308Win"
		};
		magazines[]=
		{
			"Mag_FAL_20Rnd",

		};
		magazineSwitchTime=0.2;
		barrelArmor=3000;
		ejectType=1;
		recoilModifier[]={1, 5, 2};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\SKS\SKS_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\AK74\ak74_reload",
			0.80000001,
			1,
			20
		};
		hiddenSelections[]=
		{
			"camo"
		};
		modes[]=
		{
			"SemiAuto"
		};
		class SemiAuto: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"FNFAL_Shot_SoundSet",
				"FNFAL_Tail_SoundSet",
				"FNFAL_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"AK_silencer_SoundSet",
					"AK_silencerTail_SoundSet",
					"AK_silencerInteriorTail_SoundSet"
				},
				
				{
					"FNFAL_silencerHomeMade_SoundSet",
					"FNFAL_silencerHomeMadeTail_SoundSet",
					"FNFAL_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			soundBegin[]=
			{
				"begin1",
				0.33333001,
				"begin2",
				0.33333001,
				"begin2",
				0.33333001
			};
			reloadTime=0.1;
			recoil="recoil_FAL";
			recoilProne="recoil_FAL_prone";
			dispersion=0.0007;
			magazineSlot="magazine";
			soundBeginExt[]=
			{
				
				{
					"beginSilenced_Pro",
					1
				},
				
				{
					"beginSilenced_HomeMade",
					1
				}
			};
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class Weapon_Movement_Rifle_Walk
				{
					soundSet="Weapon_Movement_Rifle_Walk_SoundSet";
					id=101;
				};
				class Weapon_Movement_Rifle_Run
				{
					soundSet="Weapon_Movement_Rifle_Run_SoundSet";
					id=102;
				};
				class Weapon_Movement_Rifle_Sprint
				{
					soundSet="Weapon_Movement_Rifle_Sprint_SoundSet";
					id=103;
				};
				class Weapon_Movement_Rifle_Land
				{
					soundSet="Weapon_Movement_Rifle_Land_SoundSet";
					id=104;
				};
				class Char_Gestures_Hand_Grab_Rifle
				{
					soundSet="Char_Gestures_Hand_Grab_FabricRifle_SoundSet";
					id=892;
				};
			};
		};
	};
	class GP_FAL: GP_FAL_Base
	{
		scope=2;
		displayName="FN FAL";
		descriptionShort="The right arm of the free world.";
		model="\dz\weapons\firearms\fal\fal_rifle.p3d";
		attachments[]=
		{
			"weaponWrap",
			"suppressorImpro",
			"weaponButtstockFal",
			"weaponOptics"
		};
		baseAttachments[]=
		{
			"Fal_OeBttstck"
		};
		randomAttachments[]=
		{
			
			{
				"BUISOptic",
				"M68Optic",
				"M4_T3NRDSOptic",
				"ReflexOptic",
				"ACOGOptic",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				""
			},
			
			{
				"Fal_FoldingBttstck",
				"Fal_OeBttstck",
				"Fal_OeBttstck",
				"Fal_OeBttstck"
			},
			
			{
				"Mag_FAL_20Rnd",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				"",
				""
			}
		};
		itemSize[]={8,3};
		dexterity=2.75;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\fal\data\fal_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\fal\data\fal.rvmat"
		};
		class Particles
		{
			class OnFire
			{
				class SmokeCloud
				{
					overrideParticle="weapon_shot_winded_smoke";
					ignoreIfSuppressed=1;
				};
				class MuzzleFlash
				{
					overrideParticle="weapon_shot_akm_01";
					ignoreIfSuppressed=1;
					illuminateWorld=1;
					positionOffset[]={-0.050000001,0,0};
				};
				class ChamberSmoke
				{
					overrideParticle="weapon_shot_chamber_smoke";
					overridePoint="Nabojnicestart";
					overrideDirectionPoint="Nabojniceend";
				};
				class ChamberSmokeRaise
				{
					overrideParticle="weapon_shot_chamber_smoke_raise";
					overridePoint="Nabojnicestart";
				};
			};
			class OnOverheating
			{
				maxOverheatingValue=60;
				shotsToStartOverheating=3;
				overheatingDecayInterval=1;
				class SmokingBarrel
				{
					overrideParticle="smoking_barrel_small";
					positionOffset[]={0.1,0,0};
					onlyWithinHealthLabel[]={0,1};
				};
				class SmokingBarrelDamaged
				{
					overrideParticle="smoking_barrel";
					positionOffset[]={0.1,0,0};
					onlyWithinHealthLabel[]={2,4};
				};
				class SmokingBarrelBadlyDamaged
				{
					overridePoint="Nabojnicestart";
					overrideParticle="smoking_barrel_small";
					onlyWithinHealthLabel[]={3,4};
				};
			};
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
								"DZ\weapons\firearms\fal\data\fal.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\fal\data\fal_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\fal\data\fal_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
};
class cfgRecoils
{
	recoil_fal[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(0.5)",
		"0.0134348*(2)",
		0.079999998,
		"0.019755*(0.5)",
		"0.003056*(2)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(0.5)",
		"-0.0005*(2)",
		0.079999998,
		"-0.001177*(0.5)",
		"-0.000188*(2)",
		0.12,
		0,
		0
	};
	recoil_fal_prone[]=
	{
		0,
		0,
		0,
		0.0040000002,
		"0.036943*(0.01)",
		"0.0134348*(0.1)",
		0.0080000004,
		"0.019755*(0.01)",
		"0.003056*(0.1)",
		0.0089999996,
		0,
		0,
		0.014,
		"-0.003138*(0.01)",
		"-0.0005*(0.1)",
		0.0080000004,
		"-0.001177*(0.01)",
		"-0.000188*(0.1)",
		0.012,
		0,
		0
	};
};
