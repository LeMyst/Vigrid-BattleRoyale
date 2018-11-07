class CfgPatches
{
	class DZ_Weapons_Firearms_SVD
	{
		units[]=
		{
			"SVD"
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
	class SVD;
	class GP_Blaze95_Base: SVD
	{
		scope=0;
		weight=2820;
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
			"Ammo_308Win",
		};
		magazines[]=
		{
			"Mag_308WinSnapLoader_2Rnd"
		};
		magazineSwitchTime=0.4;
		barrelArmor=300;
		ejectType=1;
		recoilModifier[]={1, 5, 2};
		drySound[]=
		{
			"dz\sounds\weapons\shotguns\Izh43\izh43_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\B95\b95_reload2",
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
			"Double"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"B95_Shot_SoundSet",
				"B95_Tail_SoundSet",
				"B95_InteriorTail_SoundSet"
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
			recoil="recoil_b95";
			recoilProne="recoil_b95_prone";
			dispersion=0.001;
			magazineSlot="magazine";
		};
		class Double: Mode_FullAuto
		{
			soundSetShot[]=
			{
				"B95_Shot_SoundSet",
				"B95_Tail_SoundSet",
				"B95_InteriorTail_SoundSet"
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
			reloadTime=0.0099999998;
			recoil="recoil_b95_double";
			recoilProne="recoil_b95";
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
	class GP_Blaze95: GP_Blaze95_Base
	{
		scope=2;
		displayName="B95";
		descriptionShort="Double barrelled hunting rifle. It is equipped with a single lock action. The rifle is uncocked after every shot. The notches that hold the mounting base are positioned directly above the chambers. The barrels float freely while the point of impact remains constant.";
		model="\dz\weapons\firearms\B95\b95.p3d";
		hiddenSelections[] = {"camo","zasleh"};
		attachments[]=
		{
			"weaponWrap",
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
				""
			},
			
			{
				"Mag_GPBlaze95_2Rnd",
				"",
				"",
				""
			}
		};
		itemSize[]={7,3};
		dexterity=2.65;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\B95\data\b95_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\B95\data\b95.rvmat"
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
	class GP_Blaze95_Black: GP_Blaze95
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_B95_Black0";
		color="Black";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.15,0.15,0.15,1.0,CO)"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\B95\data\b95_painted.rvmat"
		};
	};
	class GP_Blaze95_Green: GP_Blaze95
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_B95_Green0";
		color="Green";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.35,0.36,0.28,1.0,CO)"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\B95\data\b95_painted.rvmat"
		};
	};
};
