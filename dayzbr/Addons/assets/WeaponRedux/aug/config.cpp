class CfgPatches
{
	class DZ_Weapons_Firearms_UMP
	{
		units[]=
		{
			"UMP45"
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
	class UMP45;
	class GP_AugSteyr_Base: UMP45
	{
		scope=0;
		lootTag[]=
		{
			"Military_west"
		};
		weight=3600;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		//discreteDistance[]={25};
		//discreteDistanceInitIndex=0;
		modelOptics="-";
		opticsZoomMin=0.28;
		opticsZoomMax=0.28;
		opticsZoomInit=0.28;
		opticsPPEffects[]=
		{
			"-"
		};
		optics=1;
		opticsFlare=1;
		distanceZoomMin=300;
		distanceZoomMax=300;
		value=0;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_556x45"
		};
		magazines[]=
		{
			"Mag_STANAG_30Rnd",
			"Mag_STANAGCoupled_30Rnd",
			"Mag_CMAG_10Rnd",
			"Mag_CMAG_20Rnd",
			"Mag_CMAG_30Rnd",
			"Mag_CMAG_40Rnd",
			"Mag_CMAG_10Rnd_Green",
			"Mag_CMAG_20Rnd_Green",
			"Mag_CMAG_30Rnd_Green",
			"Mag_CMAG_40Rnd_Green",
			"Mag_CMAG_10Rnd_Black",
			"Mag_CMAG_20Rnd_Black",
			"Mag_CMAG_30Rnd_Black",
			"Mag_CMAG_40Rnd_Black"
		};
		magazineSwitchTime=0.38;
		barrelArmor=2390;
		ejectType=1;
		recoilModifier[]={1,1,1};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\m4a1\m4_dry",
			0.5,
			1,
			20
		};
		discreteDistance[]={300};
		discreteDistanceInitIndex=1;
		reloadAction="ReloadM4";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\m4a1\m4_reload_0",
			0.80000001,
			1,
			20
		};
		hiddenSelections[]=
		{
			"camo",
			"camo1",
			"camo2",
			"camo3",
			"camo4",
			"zasleh"
		};
		modes[]=
		{
			"FullAuto",
			"Single"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"M4_Shot_SoundSet",
				"M4_Tail_SoundSet",
				"M4_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"M4_silencer_SoundSet",
					"M4_silencerTail_SoundSet",
					"M4_silencerInteriorTail_SoundSet"
				},
				
				{
					"M4_silencerHomeMade_SoundSet",
					"M4_silencerHomeMadeTail_SoundSet",
					"M4_silencerInteriorHomeMadeTail_SoundSet"
				}
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
			reloadTime=0.085000001;
			recoil="recoil_m4";
			recoilProne="recoil_m4_prone";
			dispersion=0.0015;
			magazineSlot="magazine";
			soundBeginExt[]=
			{
				
				{
					"beginSilenced_Pro1",
					0.5,
					"beginSilenced_Pro2",
					0.5
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
				"M4_Shot_SoundSet",
				"M4_Tail_SoundSet",
				"M4_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"M4_silencer_SoundSet",
					"M4_silencerTail_SoundSet",
					"M4_silencerInteriorTail_SoundSet"
				},
				
				{
					"M4_silencerHomeMade_SoundSet",
					"M4_silencerHomeMadeTail_SoundSet",
					"M4_silencerInteriorHomeMadeTail_SoundSet"
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
			reloadTime=0.090000004;
			recoil="recoil_m4";
			recoilProne="recoil_m4_prone";
			dispersion=0.0015;
			magazineSlot="magazine";
			soundBeginExt[]=
			{
				
				{
					"beginSilenced_Pro1",
					0.5,
					"beginSilenced_Pro2",
					0.5
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
	class GP_AugSteyr: GP_AugSteyr_Base
	{
		scope=2;
		displayName="$STR_cfgWeapons_AugSteyr0";
		descriptionShort="$STR_cfgWeapons_AugSteyr1";
		model="\dz\weapons\firearms\aug\aug.p3d";
		attachments[]=
		{
			"weaponMuzzleM4",
			"suppressorImpro",
			"weaponBarrelAug"
		};
		randomAttachments[]=
		{
			
			{
				"M4_Suppressor",
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
				"Mag_STANAG_30Rnd",
				"Mag_CMAG_10Rnd",
				"Mag_CMAG_20Rnd",
				"Mag_CMAG_30Rnd",
				"Mag_CMAG_40Rnd",
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
			}
		};
		itemSize[]={8,3};
		dexterity=3;
		spawnDamageRange[]={0,0.60000002};
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
								"DZ\weapons\firearms\aug\data\aug_barrel_short.rvmat",
								"DZ\weapons\firearms\aug\data\aug_barrel_base.rvmat",
								"DZ\weapons\firearms\aug\data\aug_stock.rvmat",
								"DZ\weapons\firearms\aug\data\scope.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\aug\data\aug_barrel_short_damage.rvmat",
								"DZ\weapons\firearms\aug\data\aug_barrel_base_damage.rvmat",
								"DZ\weapons\firearms\aug\data\aug_stock_damage.rvmat",
								"DZ\weapons\firearms\aug\data\scope_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\aug\data\aug_barrel_short_destruct.rvmat",
								"DZ\weapons\firearms\aug\data\aug_barrel_base_destruct.rvmat",
								"DZ\weapons\firearms\aug\data\aug_stock_destruct.rvmat",
								"DZ\weapons\firearms\aug\data\scope_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
};
