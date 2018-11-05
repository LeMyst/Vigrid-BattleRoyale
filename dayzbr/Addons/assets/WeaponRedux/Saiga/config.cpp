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
	class GP_Saiga12KShotgun_Base: AKM
	{
		scope=0;
		lootTag[]=
		{
			"Military_east"
		};
		weight=3600;
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
			"KashtanOptic",
			"KobraOptic"
		};
		value=0;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_12gaPellets",
			"Ammo_12gaSlug"
		};
		magazines[]=
		{
			"Mag_Saiga_5Rnd",
			"Mag_Saiga_8Rnd",
			"Mag_Saiga_Drum20Rnd"
		};
		hiddenSelections[]=
		{
			"camo",
			"zasleh"
		};
		magazineSwitchTime=0.2;
		barrelArmor=3000;
		ejectType=1;
		recoilModifier[]={1, 4, 1.9};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\akm\Akm_reload",
			0.80000001,
			1,
			20
		};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\SKS\SKS_dry",
			0.5,
			1,
			20
		};
		modes[]=
		{
			"Single"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"Mp133_Shot_SoundSet",
				"Mp133_Tail_SoundSet",
				"Mp133_InteriorTail_SoundSet"
			};
			begin1[]=
			{
				"dz\sounds\weapons\shotguns\mp133\mp133_single_0",
				1,
				1,
				800
			};
			begin2[]=
			{
				"dz\sounds\weapons\shotguns\mp133\mp133_single_1",
				1,
				1,
				800
			};
			begin3[]=
			{
				"dz\sounds\weapons\shotguns\mp133\mp133_single_2",
				1,
				1,
				800
			};
			soundBegin[]=
			{
				"begin1",
				0.33333001,
				"begin2",
				0.33333001,
				"begin3",
				0.33333001
			};
			reloadTime=0.1;
			recoil="recoil_Saiga12";
			recoilProne="recoil_Saiga12_prone";
			dispersion=0.0099999998;
			firespreadangle=1.5;
			magazineSlot="magazine";
			beginSilenced_Pro[]=
			{
				"dz\sounds\weapons\firearms\AK101\akSilenced",
				1,
				1,
				200
			};
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\AK101\akSilenced",
				1,
				1,
				300
			};
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
	class GP_Saiga12KShotgun: GP_Saiga12KShotgun_Base
	{
		scope=2;
		displayName="Saiga-12K";
		descriptionShort="A combat shotgun which the internal mechanism and appearance was based on Kalashnikov 's AK rifles. It is gas operated with a rotating bolt. The Saiga-12 is semi-automatic fire only shotgun with foldable metal frame-skeleton buttstock. It can accept 5-8rounds detachable magazines, and 20 rounds detachable drum-type magazine.";
		model="\dz\weapons\shotguns\saiga\saiga.p3d";
		attachments[]=
		{
			"weaponButtstockSaiga",
			"weaponOpticsAK",
			"weaponWrap"
		};
		baseAttachments[]=
		{
			"Saiga_Bttstck"
		};
		randomAttachments[]=
		{
			
			{
				"KashtanOptic",
				"PSO11Optic",
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
				"Mag_Saiga_5Rnd",
				"Mag_Saiga_8Rnd",
				"Mag_Saiga_Drum20Rnd",
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
			}
		};
		dexterity=2.7;
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
								"DZ\weapons\shotguns\saiga\Data\saiga.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\shotguns\saiga\Data\saiga_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\shotguns\saiga\Data\saiga_destruct.rvmat"
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
	recoil_ShotgunSaiga12[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(1)",
		"0.0134348*(3)",
		0.079999998,
		"0.019755*(1)",
		"0.003056*(3)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(1)",
		"-0.0005*(3)",
		0.079999998,
		"-0.001177*(1)",
		"-0.000188*(3)",
		0.12,
		0,
		0
	};
	recoil_ShotgunSaiga12_prone[]=
	{
		0,
		0,
		0,
		0.0040000002,
		"0.036943*(0.5)",
		"0.0134348*(2)",
		0.0080000004,
		"0.019755*(0.5)",
		"0.003056*(2)",
		0.0089999996,
		0,
		0,
		0.014,
		"-0.003138*(0.5)",
		"-0.0005*(2)",
		0.0080000004,
		"-0.001177*(0.5)",
		"-0.000188*(2)",
		0.012,
		0,
		0
	};
};

