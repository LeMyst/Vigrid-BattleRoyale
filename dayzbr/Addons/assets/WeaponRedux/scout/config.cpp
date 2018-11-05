class CfgPatches
{
	class DZ_Weapons_Firearms_MosinNagant
	{
		units[]=
		{
			"Mosin9130"
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
	class Mosin9130;
	class GP_Scout_Base: Mosin9130
	{
		scope=0;
		weight=2720;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		ContinuousActions[]=
		{
			"AT_LOAD_MULTI_BULLET_TO_WEAPON"
		};
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
		chamberSize=5;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_308Win"
		};
		magazineSwitchTime=0.38;
		barrelArmor=900;
		ejectType=1;
		recoilModifier[]={1, 5, 2.25};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\mosin9130\mosin_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\cz527\cz527_reload_0",
			0.80000001,
			1,
			30
		};
		reloadSound[]=
		{
			"dz\sounds\weapons\firearms\cz527\cz527_cycling_0",
			0.80000001,
			1,
			30
		};
		hiddenSelections[]=
		{
			"camo",
			"zasleh"
		};
		modes[]=
		{
			"Single"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"CR527_Shot_SoundSet",
				"CR527_Tail_SoundSet",
				"CR527_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"CR527_silencerHomeMade_SoundSet",
					"CR527_silencerHomeMadeTail_SoundSet",
					"CR527_silencerInteriorTail_SoundSet"
				}
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\cz527\cz527_single_0",,
				1,
				1,
				1000
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\cz527\cz527_single_1",
				1,
				1,
				1000
			};
			begin3[]=
			{
				"dz\sounds\weapons\firearms\cz527\cz527_single_2",
				1,
				1,
				1000
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
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\m4a1\m4Silenced",
				1,
				1,
				150
			};
			soundBeginExt[]=
			{
				
				{
					"beginSilenced_HomeMade",
					1
				}
			};
			reloadTime=2;
			recoil="recoil_scout";
			recoilProne="recoil_scout_prone";
			dispersion=0.001;
			magazineSlot="magazine";
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
	class GP_Scout: GP_Scout_Base
	{
		scope=2;
		displayName="Scout Rifle";
		descriptionShort="The Scout Rifle provides the absolute solution for both versatility and accuracy in a lightweight bolt-action rifle platform.";
		model="\dz\weapons\firearms\scout\scout.p3d";
		attachments[]=
		{
			"weaponOptics",
			"weaponWrap",
			"suppressorImpro",
			"weaponOpticsHunting"
		};
		baseAttachments[]={};
		randomAttachments[]=
		{
			
			{
				"M68Optic",
				"ACOGOptic",
				"ACOGOptic",
				"ACOGOptic",
				"ACOGOptic",
				"ACOGOptic"
			},
			
			{
				"GP_Mag_Scout_5Rnd",
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
		dexterity=2.7;
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\scout\data\scout.rvmat"
		};
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\scout\data\scout_blk_co.paa"
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
								"DZ\weapons\firearms\scout\data\scout.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\scout\data\scout_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\scout\data\scout_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class GP_Scout_Black: GP_Scout
	{
		scope=2;
		descriptionShort="The Scout Rifle provides the absolute solution for both versatility and accuracy in a lightweight bolt-action rifle platform. This one comes with black color stock.";
		color="Black";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\scout\data\scout_co.paa"
		};
	};
	class GP_Scout_Green: GP_Scout
	{
		scope=2;
		descriptionShort="The Scout Rifle provides the absolute solution for both versatility and accuracy in a lightweight bolt-action rifle platform. This one comes with green color stock.";
		color="Green";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\scout\data\scout_grn_co.paa"
		};
	};
};
class cfgRecoils
{
	recoil_scout[]=
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
	recoil_scout_prone[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(0.3)",
		"0.0134348*(1)",
		0.079999998,
		"0.019755*(0.3)",
		"0.003056*(1)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(0.3)",
		"-0.0005*(1)",
		0.079999998,
		"-0.001177*(0.3)",
		"-0.000188*(1)",
		0.12,
		0,
		0
	};
};