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
	class akm;
	class GP_AK101_Base: akm
	{
		scope=0;
		weight=3077;
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
			"Ammo_556x45"
		};
		magazines[]=
		{
			"Mag_AK101_30Rnd"
		};
		magazineSwitchTime=0.2;
		barrelArmor=2560;
		ejectType=1;
		recoilModifier[]={1, 3.8, 1.9};
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
			"SemiAuto",
			"FullAuto"
		};
		class SemiAuto: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"AK_Shot_SoundSet",
				"AK_Tail_SoundSet",
				"AK_InteriorTail_SoundSet"
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
			recoil="recoil_AK101";
			recoilProne="recoil_AK101_prone";
			dispersion=0.0015;
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
		class FullAuto: Mode_FullAuto
		{
			soundSetShot[]=
			{
				"AK_Shot_SoundSet",
				"AK_Tail_SoundSet",
				"AK_InteriorTail_SoundSet"
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
			recoil="recoil_AKM";
			recoilProne="recoil_AKM_prone";
			dispersion=0.0015;
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
	class GP_AK101: GP_AK101_Base
	{
		scope=2;
		displayName="AK101";
		descriptionShort="An assault rifle of the Kalashnikov series designed for the world export market. It is designed with modern and composite materials, including plastics that reduce weight and improve accuracy. The disassembly procedure for the AK-101 is identical to that of the AK-74. The AK-101 has an attachment rail installed on the side of the receiver for mounting scopes and other optical sights, which will accept most types of Russian and European AK optics. The AK-101 has a 415 mm (16.3 in) barrel with an AK-74 style muzzle brake attached to the barrel to control muzzle climb.";
		model="\dz\weapons\firearms\AK101\ak101.p3d";
		attachments[]=
		{
			"weaponButtstockAK",
			"weaponHandguardAK",
			"weaponOpticsAK",
			"weaponFlashlight",
			"weaponBipod",
			"weaponWrap",
			"weaponMuzzleAK",
			"weaponBayonetAK",
			"suppressorImpro"
		};
		baseAttachments[]=
		{
			"AK_PlasticBttstck",
			"AK_PlasticHndgrd"
		};
		randomAttachments[]=
		{
			
			{
				"AK_FoldingBttstck",
				"Att_Buttstock_AK_Wood",
				"AK_WoodBttstck",
				"AK_PlasticBttstck",
				"AK_PlasticBttstck",
				"AK_PlasticBttstck",
				"AK_PlasticBttstck",
				"AK_PlasticBttstck"
			},
			
			{
				"AK_WoodHndgrd",
				"AK_PlasticHndgrd",
				"AK_PlasticHndgrd",
				"AK_PlasticHndgrd",
				"AK_PlasticHndgrd"
			},
			
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
				"",
				"",
				"",
				"",
				""
			},
			
			{
				"AK_Suppressor",
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
				"Mag_AK101_30Rnd",
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
		itemSize[]={7,3};
		dexterity=2.75;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\AK101\data\ak101_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\AK101\data\AK101.rvmat"
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
								"DZ\weapons\firearms\AK101\data\AK101.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\AK101\data\AK101_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\AK101\data\AK101_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class GP_AK101_Black: GP_AK101
	{
		scope=2;
		descriptionShort="An assault black painted rifle of the Kalashnikov series designed for the world export market. It is designed with modern and composite materials, including plastics that reduce weight and improve accuracy. The disassembly procedure for the AK-101 is identical to that of the AK-74. The AK-101 has an attachment rail installed on the side of the receiver for mounting scopes and other optical sights, which will accept most types of Russian and European AK optics. The AK-101 has a 415 mm (16.3 in) barrel with an AK-74 style muzzle brake attached to the barrel to control muzzle climb.";
		color="Black";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.12,0.12,0.12,1.0,CO)"
		};
	};
	class GP_AK101_Green: GP_AK101
	{
		scope=2;
		descriptionShort="An assault green painted rifle of the Kalashnikov series designed for the world export market. It is designed with modern and composite materials, including plastics that reduce weight and improve accuracy. The disassembly procedure for the AK-101 is identical to that of the AK-74. The AK-101 has an attachment rail installed on the side of the receiver for mounting scopes and other optical sights, which will accept most types of Russian and European AK optics. The AK-101 has a 415 mm (16.3 in) barrel with an AK-74 style muzzle brake attached to the barrel to control muzzle climb.";
		color="Green";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.35,0.36,0.28,1.0,CO)"
		};
	};
};
class cfgRecoils
{
	recoil_AK101[]=
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
	recoil_AK101_prone[]=
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
