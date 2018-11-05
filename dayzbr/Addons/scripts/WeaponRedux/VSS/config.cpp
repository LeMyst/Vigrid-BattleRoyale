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
	class GP_VSS_Base: AKM
	{
		scope=0;
		weight=1990;
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
			"GrozaOptic",
			"KobraOptic"
		};
		value=0;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_9x39"
		};
		magazines[]=
		{
			"Mag_VSS_10Rnd"
		};
		magazineSwitchTime=0.38;
		barrelArmor=3000;
		ejectType=1;
		recoilModifier[]={1, 1.75, 1.25};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\SKS\SKS_dry",
			0.5,
			1,
			20
		};
		//reloadAction="ReloadAKM";
		reloadAction="ReloadVSS";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\akm\Akm_reload",
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
			"FullAuto"
		};
		class FullAuto: Mode_FullAuto
		{
			soundSetShot[]=
			{
				"UMP45_silencerPro_SoundSet",
				"UMP45_silencerTail_SoundSet",
				"UMP45_silencerInteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"AK_silencer_SoundSet",
					"AK_silencerTail_SoundSet",
					"AK_silencerInteriorTail_SoundSet"
				},
				
				{
					"AK_silencerHomeMade_SoundSet",
					"AK_silencerHomeMadeTail_SoundSet",
					"AK_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\AK101\AkSilenced",
				1,
				1,
				100
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\AK101\AkSilenced",
				1,
				1,
				100
			};
			begin3[]=
			{
				"dz\sounds\weapons\firearms\AK101\AkSilenced",
				1,
				1,
				100
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
			recoil="recoil_VSS";
			recoilProne="recoil_VSS_prone";
			dispersion=0.0020000001;
			magazineSlot="magazine";
			beginSilenced_Pro[]=
			{
				"dz\sounds\weapons\firearms\AK101\akSilenced",
				0.30000001,
				1,
				75
			};
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\AK101\akSilenced",
				0.30000001,
				1,
				100
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
	class GP_VSS: GP_VSS_Base
	{
		scope=2;
		displayName="VSS Vintorez";
		descriptionShort="Integrally supressed sniper rifle, developed in late 80's Soviet Union. Issued to Spetznaz units for clandestine operations.";
		model="\dz\weapons\firearms\VSS\VSS.p3d";
		baseAttachments[]=
		{
			"PSO11Optic"
		};
		attachments[]=
		{
			"weaponOpticsAK",
			"weaponWrap"
		};
		randomAttachments[]=
		{
			
			{
				"KashtanOptic",
				"PSO1Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic",
				"PSO11Optic"
			},
			
			{
				"Mag_VSS_10Rnd",
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
		itemSize[]={9,6};
		dexterity=2.75;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\VSS\data\vss_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\VSS\data\vss.rvmat"
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
								"DZ\weapons\firearms\VSS\Data\vss.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\VSS\Data\vss_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\VSS\Data\vss_destruct.rvmat"
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
	recoil_VSS[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(0.4)",
		"0.0134348*(1.2)",
		0.079999998,
		"0.019755*(0.4)",
		"0.003056*(1.2)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(0.4)",
		"-0.0005*(1.2)",
		0.079999998,
		"-0.001177*(0.4)",
		"-0.000188*(1.2)",
		0.12,
		0,
		0
	};
	recoil_VSS_prone[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(0.2)",
		"0.0134348*(0.6)",
		0.079999998,
		"0.019755*(0.2)",
		"0.003056*(0.6)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(0.2)",
		"-0.0005*(0.6)",
		0.079999998,
		"-0.001177*(0.2)",
		"-0.000188*(0.6)",
		0.12,
		0,
		0
	};
};
