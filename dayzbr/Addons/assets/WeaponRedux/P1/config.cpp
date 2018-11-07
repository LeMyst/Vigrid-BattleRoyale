class CfgPatches
{
	class DZ_Weapons_Firearms_FNX45
	{
		units[]=
		{
			"FNX45"
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
	class FNX45;
	class GP_P1_Base: FNX45
	{
		scope=0;
		weight=960;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		modelOptics="-";
		distanceZoomMin=100;
		distanceZoomMax=100;
		optics=1;
		value=0;
		chamberSize=1;
		chamberedRound="";
		hiddenSelections[]=
		{
			"camo",
			"zasleh"
		};
		magazines[]=
		{
			"Mag_P1_8Rnd"
		};
		chamberableFrom[]=
		{
			"Ammo_9x19"
		};
		ejectType=1;
		recoilModifier[]={1, 1.9, 1};
		drySound[]=
		{
			"DZ\sounds\weapons\firearms\waltherP1\WaltherP1_closure",
			0.5,
			1,
			20
		};
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\waltherP1\WaltherP1_reload_00",
			0.80000001,
			1,
			20
		};
		reloadAction="ReloadRugerP1";
		modes[]=
		{
			"Single"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"WaltherP1_Shot_SoundSet",
				"WaltherP1_Tail_SoundSet",
				"WaltherP1_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"WaltherP1_silencerPro_SoundSet",
					"WaltherP1_silencerTail_SoundSet",
					"WaltherP1_silencerInteriorTail_SoundSet"
				},
				
				{
					"WaltherP1_silencerHomeMade_SoundSet",
					"WaltherP1_silencerHomeMadeTail_SoundSet",
					"WaltherP1_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			soundBegin[]=
			{
				"begin1",
				0.33333001,
				"begin2",
				0.33333001,
				"begin1",
				0.33333001,
				"begin2",
				0.33333001
			};
			reloadTime=0.13;
			recoil="recoil_p1";
			recoilProne="recoil_p1_prone";
			dispersion=0.0060000001;
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
				class Pistol_SlowHandling
				{
					soundSet="Pistol_SlowHandling_SoundSet";
					id=101;
				};
				class Pistol_ShortHandling
				{
					soundSet="Pistol_ShortHandling_SoundSet";
					id=102;
				};
				class Pistol_FastHandling
				{
					soundSet="Pistol_FastHandling_SoundSet";
					id=103;
				};
				class pickUpPistol
				{
					soundSet="pickUpPistol_SoundSet";
					id=797;
				};
			};
		};
	};
	class GP_P1: GP_P1_Base
	{
		scope=2;
		displayName="P1 Pistol";
		descriptionShort="P1 was the first locked-breech pistol to use a double-action/single-action. The shooter can chamber a round, use the de-cocking lever to safely lower the hammer without firing the round and carry the weapon loaded.";
		model="\dz\weapons\pistols\p1\p1.p3d";
		attachments[]=
		{
			"pistolMuzzle",
			"suppressorImpro"
		};
		randomAttachments[]=
		{
			
			{
				"Att_Suppressor_Pistol",
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
				"M_P1_8Rnd",
				"M_P1_8Rnd",
				"M_P1_8Rnd",
				"M_P1_8Rnd",
				"",
				"",
				"",
				"",
				"",
				""
			}
		};
		dexterity=3;
		itemSize[]={3,3};
		class Particles
		{
			class OnFire
			{
				class MuzzleFlashForward
				{
					ignoreIfSuppressed=1;
					illuminateWorld=1;
					overrideParticle="weapon_shot_fnx_01";
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
				shotsToStartOverheating=3;
				maxOverheatingValue=7;
				overheatingDecayInterval=1;
				class SmokingBarrel
				{
					overridePoint="Nabojnicestart";
					overrideParticle="smoking_barrel_small";
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
								"DZ\weapons\pistols\p1\data\p38.rvmat",
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\pistols\p1\data\p38_damage.rvmat",
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\pistols\p1\data\p38_destruct.rvmat"
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
	recoil_p1[]=
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
	recoil_p1_prone[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(0.5)",
		"0.0134348*(1)",
		0.079999998,
		"0.019755*(0.5)",
		"0.003056*(1)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(0.5)",
		"-0.0005*(1)",
		0.079999998,
		"-0.001177*(0.5)",
		"-0.000188*(1)",
		0.12,
		0,
		0
	};
};
class CfgNonAIVehicles
{
	class ProxyAttachment;
	class ProxyGlock: ProxyAttachment
	{
		scope=2;
		inventorySlot="pistol";
		model="\dz\weapons\pistols\glock\Glock19.p3d";
	};
};