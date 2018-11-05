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
	class GP_Ruger1022_Base: AKM
	{
		scope=0;
		weight=2300;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		discreteDistance[]={50};
		discreteDistanceInitIndex=0;
		modelOptics="-";
		distanceZoomMin=50;
		distanceZoomMax=50;
		PPDOFProperties[]={1,0.60000002,200000,0.30000001,3,0.1};
		optics=1;
		opticsFlare=0;
		ironsightsExcludingOptics[]={};
		forceOptics=1;
		value=0;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_22"
		};
		magazines[]=
		{
			"Mag_Ruger1022_30Rnd",
			"Mag_Ruger1022_10Rnd"
		};
		magazineSwitchTime=0.4;
		barrelArmor=300;
		ejectType=1;
		recoilModifier[]={1, 2, 1.5};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_reload",
			0.80000001,
			1,
			20
		};
		hiddenSelections[]=
		{
			"camo",
			"zasleh"
		};
		modes[]=
		{
			"Single",
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
				1,
				1,
				300
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_close_1auth",
				1,
				1,
				300
			};
			begin3[]=
			{
				"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_close_2",
				1,
				1,
				300
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
			recoil="recoil_single_primary_1outof10";
			recoilProne="recoil_single_primary_1outof10";
			dispersion=0.0025;
			magazineSlot="magazine";
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\Ruger1022\Ruger1022_close_0auth",
				1,
				1,
				300
			};
			soundBeginExt[]=
			{
				
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
	class GP_Ruger1022: GP_Ruger1022_Base
	{
		scope=2;
		displayName="Sporter 22";
		descriptionShort="A popular rifle; Its low cost, slight recoil, and low small noise signature makes it a great choice for firearms training, small game hunting, and recreational shooting.";
		model="\dz\weapons\firearms\Ruger1022\Ruger1022.p3d";
		hiddenSelections[] = {"camo","zasleh"};
		attachments[]=
		{
			"weaponWrap",
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
				"Mag_Ruger1022_30Rnd",
				"Mag_Ruger1022_10Rnd",
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
		itemSize[]={7,3};
		dexterity=3.15;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\Ruger1022\data\ruger1022_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\Ruger1022\data\ruger1022.rvmat"
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
								"DZ\weapons\firearms\Ruger1022\Data\ruger1022.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\Ruger1022\Data\ruger1022_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\Ruger1022\Data\ruger1022_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class GP_Ruger1022_Black: GP_Ruger1022
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_Ruger1022_Black0";
		color="Black";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\Ruger1022\data\ruger1022_black_co.paa"
		};
	};
	class GP_Ruger1022_Green: GP_Ruger1022
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_Ruger1022_Black0";
		color="Black";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\Ruger1022\data\ruger1022_green_co.paa"
		};
	};
};
