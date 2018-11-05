class CfgPatches
{
	class DZ_Weapons_Shotguns_MP133
	{
		units[]=
		{
			"ShotgunMp133"
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
	class Mp133Shotgun;
	class GP_Trumpet_Base: Mp133Shotgun
	{
		scope=0;
		weight=2000;
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
		value=0;
		chamberSize=7;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_22"
		};
		magazineSwitchTime=1;
		barrelArmor=600;
		ejectType=1;
		recoilModifier[]={1, 2.5, 1.5};
		drySound[]=
		{
			"dz\sounds\weapons\shotguns\Izh43\izh43_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadMP133";
		shotAction="ReloadMP133Shot";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\trumpet\trombone_reload",
			0.80000001,
			1,
			30
		};
		reloadSound[]=
		{
			"dz\sounds\weapons\firearms\trumpet\trombone_pump",
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
				"Ruger1022_Shot_SoundSet",
				"Ruger1022_Tail_SoundSet",
				"Ruger1022_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"Ruger1022_silencerHomeMade_SoundSet",
					"Ruger1022_silencerHomeMadeTail_SoundSet",
					"Ruger1022_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_close_0auth",
				0.80000001,
				1,
				175
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_close_1auth",
				0.80000001,
				1,
				175
			};
			soundBegin[]=
			{
				"begin1",
				0.5,
				"begin2",
				0.5
			};
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\m4a1\m4Silenced",
				0.050000001,
				1,
				100
			};
			soundBeginExt[]=
			{
				
				{
					"beginSilenced_HomeMade",
					1
				}
			};
			reloadTime=1;
			recoil="recoil_single_primary_1outof10";
			recoilProne="recoil_single_primary_1outof10";
			dispersion=0.0015;
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
	class GP_Trumpet: GP_Trumpet_Base
	{
		scope=2;
		displayName="Trumpet";
		descriptionShort="A pump-action rifle, which can be quickly disassembled into two pieces, significantly reducing it's length, making it easier to store, pack, transport and conceal.";
		model="\dz\weapons\firearms\Trumpet\trumpet.p3d";
		attachments[]=
		{
			"suppressorImpro",
			"weaponOpticsHunting"
		};
		baseAttachments[]={};
		randomAttachments[]=
		{
			
			{
				"HuntingOptic",
				"",
				"",
				""
			}
		};
		itemSize[]={8,4};
		dexterity=3.5;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\Trumpet\data\trombone_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\Trumpet\data\trumpet.rvmat"
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
								"DZ\weapons\firearms\Trumpet\data\trumpet.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\Trumpet\data\trumpet_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\Trumpet\data\trumpet_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
};
