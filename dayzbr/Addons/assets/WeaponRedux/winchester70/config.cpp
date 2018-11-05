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
	class GP_Winchester70_Base: Mosin9130
	{
		scope=0;
		animName="Mosin9130";
		lootTag[]=
		{
			"Civilian",
			"Hunting"
		};
		weight=2721;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		modelOptics="-";
		distanceZoomMin=100;
		distanceZoomMax=100;
		optics=1;
		opticsFlare=0;
		ContinuousActions[]=
		{
			"AT_LOAD_MULTI_BULLET_TO_WEAPON"
		};
		value=0;
		chamberSize=5;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_308Win"
		};
		magazines[]={};
		barrelArmor=1800;
		ejectType=0;
		recoilModifier[]={1, 5, 1.75};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\mosin9130\mosin_dry",
			0.5,
			1,
			20
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
			"DZ\sounds\weapons\firearms\mosin9130\mosin_cycling",
			0.80000001,
			1,
			20
		};
		reloadAction="ReloadMosinFull";
		shotAction="ReloadMosinShort";
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
				"Win_Shot_SoundSet",
				"Win_Tail_SoundSet",
				"Win_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"Win_silencerHomeMade_SoundSet",
					"Win_silencerHomeMadeTail_SoundSet",
					"Win_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			soundBegin[]=
			{
				"begin1",
				0.5,
				"begin2",
				0.5
			};
			soundBeginExt[]=
			{
				
				{
					"beginSilenced_HomeMade",
					1
				}
			};
			reloadTime=2;
			recoil="recoil_Winchester";
			recoilProne="recoil_Winchester_prone";
			dispersion=0.001;
			magazineSlot="magazine";
		};
		class Particles
		{
			class OnFire
			{
				class MuzzleFlash
				{
					ignoreIfSuppressed=1;
					overridePoint="Usti hlavne";
				};
				class ChamberFlash
				{
					overridePoint="Usti hlavne";
					illuminateWorld=1;
					overrideParticle="weapon_shot_izh18_01";
				};
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
	class GP_Winchester70: GP_Winchester70_Base
	{
		scope=2;
		displayName="Winchester model 70";
		descriptionShort="Winchester model 70 is a bolt action sporting rifle with an iconic place in American sporting culture and has been held in high regard by shooters ever since it was introduced. Trigger creep has been virtually eliminated in the new Model 70, it also uses M.O.A. trigger system that has no take up.";
		model="\dz\weapons\firearms\winchester70\winchester70.p3d";
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
			}
		};
		itemSize[]={9,6};
		dexterity=3.3;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\firearms\winchester70\data\winchester70_CO.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\firearms\winchester70\data\winchester70.rvmat"
		};
		class Damage
		{
			tex[]={};
			mat[]=
			{
				"DZ\weapons\firearms\winchester70\data\winchester70.rvmat",
				"DZ\weapons\firearms\winchester70\data\winchester70_damage.rvmat",
				"DZ\weapons\firearms\winchester70\data\winchester70_destruct.rvmat"
			};
		};
	};
	class GP_Winchester70_Black: GP_Winchester70
	{
		scope=2;
		descriptionShort="Winchester model 70 is a bolt action sporting rifle with an iconic place in American sporting culture and has been held in high regard by shooters ever since it was introduced. Trigger creep has been virtually eliminated in the new Model 70, it also uses M.O.A. trigger system that has no take up. This one is black painted.";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.15,0.15,0.15,1.0,CO)"
		};
	};
	class GP_Winchester70_Green: GP_Winchester70
	{
		scope=2;
		descriptionShort="Winchester model 70 is a bolt action sporting rifle with an iconic place in American sporting culture and has been held in high regard by shooters ever since it was introduced. Trigger creep has been virtually eliminated in the new Model 70, it also uses M.O.A. trigger system that has no take up. This one is green painted.";
		lootCategory="Crafted";
		hiddenSelectionsTextures[]=
		{
			"#(argb,8,8,3)color(0.35,0.36,0.28,1.0,CO)"
		};
	};
};
class cfgRecoils
{
	recoil_Winchester[]=
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
	recoil_Winchester_prone[]=
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
