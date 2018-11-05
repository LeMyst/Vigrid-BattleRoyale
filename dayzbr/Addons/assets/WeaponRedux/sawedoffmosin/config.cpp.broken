class CfgPatches
{
	class DZ_Weapons_Firearms_MosinNagant
	{
		units[]=
		{
			"Mosin9130",
			"Mosin9130_Black",
			"Mosin9130_Green",
			"Mosin9130_Green_Black",
			"Mosin9130_Black_Green"
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
	class GP_SawedoffMosin9130_Base: Mosin9130
	{
		scope=0;
		animName="Mosin9130";
		weight=3000;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		modelOptics="-";
		distanceZoomMin=100;
		distanceZoomMax=100;
		discreteDistance[]={50};
		discreteDistanceInitIndex=0;
		optics=1;
		opticsFlare=0;
		value=0;
		chamberSize=5;
		chamberedRound="";
		magazines[]={};
		chamberableFrom[]=
		{
			"Ammo_762x54",
			"Ammo_762x54Tracer",
			"Mag_CLIP762x54_5Rnd"
		};
		cursor="aimBowGhost";
		barrelArmor=300;
		ejectType=0;
		recoilModifier[]={1,1,1};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\mosin9130\mosin_dry",
			0.5,
			1
		};
		reloadSkips[]={0.31999999,0.41,0.5,0.58999997,0.69,0.76999998};
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\mosin9130\mosin_reloading",
			0.80000001,
			1,
			20
		};
		reloadSound[]=
		{
			"dz\sounds\weapons\firearms\mosin9130\mosin_cycling",
			0.80000001,
			1,
			20
		};
		reloadAction="ReloadMosinFull";
		shotAction="ReloadMosinShort";
		hiddenSelections[]=
		{
			"camo"
		};
		modes[]=
		{
			"Single"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"Mosin_Shot_SoundSet",
				"Mosin_Tail_SoundSet",
				"Mosin_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"Mosin_silencerHomeMade_SoundSet",
					"Mosin_silencerHomeMadeTail_SoundSet",
					"Mosin_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\mosin9130\mosin_close_0",
				1,
				1,
				1000
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\mosin9130\mosin_close_1",
				1,
				1,
				1000
			};
			begin3[]=
			{
				"dz\sounds\weapons\firearms\mosin9130\mosin_close_2",
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
			recoil="recoil_mosin";
			recoilProne="recoil_mosin_prone";
			dispersion=0.003;
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
				class mosin9130_charge_open
				{
					soundSet="mosin9130_charge_open_SoundSet";
					id=1;
				};
				class mosin9130_chamber_load
				{
					soundSet="mosin9130_chamber_load_SoundSet";
					id=2;
				};
				class mosin9130_charge_close
				{
					soundSet="mosin9130_charge_close_SoundSet";
					id=3;
				};
				class mosin9130_dry
				{
					soundSet="mosin9130_dry_SoundSet";
					id=10;
				};
				class mosin9130_pullout
				{
					soundSet="mosin9130_pullout_SoundSet";
					id=11;
				};
			};
		};
	};
	class GP_SawedoffMosin9130: GP_SawedoffMosin9130_Base
	{
		scope=2;
		displayName="$STR_cfgWeapons_SawedoffMosin91300";
		descriptionShort="$STR_cfgWeapons_SawedoffMosin91301";
		model="\dz\weapons\firearms\mosin9130\mosin_sawn.p3d";
		attachments[]=
		{
			"weaponOpticsMosin"
		};
		baseAttachments[]={};
		lootCategory="Crafted";
		itemSize[]={6,5};
		dexterity=2.7;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\mosin9130\data\mosin_sawn_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\mosin9130\data\mosin_sawn.rvmat"
		};
		class Damage
		{
			tex[]={};
			mat[]=
			{
				"DZ\weapons\firearms\mosin9130\data\mosin_sawn.rvmat",
				"DZ\weapons\firearms\mosin9130\data\mosin_sawn_damage.rvmat",
				"DZ\weapons\firearms\mosin9130\data\mosin_sawn_destruct.rvmat"
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
				class mosin9130_charge_open
				{
					soundSet="mosin9130_charge_open_SoundSet";
					id=1;
				};
				class mosin9130_chamber_load
				{
					soundSet="mosin9130_chamber_load_SoundSet";
					id=2;
				};
				class mosin9130_charge_close
				{
					soundSet="mosin9130_charge_close_SoundSet";
					id=3;
				};
				class mosin9130_dry
				{
					soundSet="mosin9130_dry_SoundSet";
					id=10;
				};
				class mosin9130_pullout
				{
					soundSet="mosin9130_pullout_SoundSet";
					id=11;
				};
			};
		};
	};
	class GP_SawedoffMosin9130_Black: GP_SawedoffMosin9130
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_SawedoffMosin9130_Black0";
		color="Black";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.15,0.15,0.15,1.0,CO)"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\mosin9130\data\mosin_sawn_bk.rvmat"
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
				class mosin9130_charge_open
				{
					soundSet="mosin9130_charge_open_SoundSet";
					id=1;
				};
				class mosin9130_chamber_load
				{
					soundSet="mosin9130_chamber_load_SoundSet";
					id=2;
				};
				class mosin9130_charge_close
				{
					soundSet="mosin9130_charge_close_SoundSet";
					id=3;
				};
				class mosin9130_dry
				{
					soundSet="mosin9130_dry_SoundSet";
					id=10;
				};
				class mosin9130_pullout
				{
					soundSet="mosin9130_pullout_SoundSet";
					id=11;
				};
			};
		};
	};
	class GP_SawedoffMosin9130_Green: GP_SawedoffMosin9130
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_SawedoffMosin9130_Green0";
		color="Green";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.35,0.36,0.28,1.0,CO)"
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
				class mosin9130_charge_open
				{
					soundSet="mosin9130_charge_open_SoundSet";
					id=1;
				};
				class mosin9130_chamber_load
				{
					soundSet="mosin9130_chamber_load_SoundSet";
					id=2;
				};
				class mosin9130_charge_close
				{
					soundSet="mosin9130_charge_close_SoundSet";
					id=3;
				};
				class mosin9130_dry
				{
					soundSet="mosin9130_dry_SoundSet";
					id=10;
				};
				class mosin9130_pullout
				{
					soundSet="mosin9130_pullout_SoundSet";
					id=11;
				};
			};
		};
	};
	class GP_SawedoffMosin9130_Camo: GP_SawedoffMosin9130
	{
		scope=2;
		descriptionShort="$STR_cfgWeapons_SawedoffMosin9130_Camo0";
		color="Camo";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\mosin9130\data\mosin_nagant_camo_co.paa"
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
				class mosin9130_charge_open
				{
					soundSet="mosin9130_charge_open_SoundSet";
					id=1;
				};
				class mosin9130_chamber_load
				{
					soundSet="mosin9130_chamber_load_SoundSet";
					id=2;
				};
				class mosin9130_charge_close
				{
					soundSet="mosin9130_charge_close_SoundSet";
					id=3;
				};
				class mosin9130_dry
				{
					soundSet="mosin9130_dry_SoundSet";
					id=10;
				};
				class mosin9130_pullout
				{
					soundSet="mosin9130_pullout_SoundSet";
					id=11;
				};
			};
		};
	};
};
class cfgRecoils
{
	recoil_mosin[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(0.8)",
		"0.0134348*(2.3)",
		0.079999998,
		"0.019755*(0.8)",
		"0.003056*(2.3)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(0.8)",
		"-0.0005*(2.3)",
		0.079999998,
		"-0.001177*(0.8)",
		"-0.000188*(2.3)",
		0.12,
		0,
		0
	};
	recoil_mosin_prone[]=
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
