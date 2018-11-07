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
	class GP_Glock19_Base: FNX45
	{
		scope=0;
		lootTag[]=
		{
			"Police"
		};
		weight=1000;
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
			"Mag_Glock_15Rnd"
		};
		chamberableFrom[]=
		{
			"Ammo_9x19"
		};
		ejectType=1;
		recoilModifier[]={1, 1.9, 1};
		drySound[]=
		{
			"DZ\sounds\weapons\firearms\glock19\Glock19_dry",
			0.5,
			1,
			20
		};
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\FNX45\FNX45_reload",
			0.80000001,
			1,
			20
		};
		reloadAction="ReloadGlock";
		modes[]=
		{
			"Single"
		};
		class Single: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"Glock19_Shot_SoundSet",
				"Glock19_Tail_SoundSet",
				"Glock19_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"Glock19_silencerPro_SoundSet",
					"Glock19_silencerTail_SoundSet",
					"Glock19_silencerInteriorTail_SoundSet"
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
			reloadTime=0.13;
			recoil="recoil_Glock";
			recoilProne="recoil_Glock_prone";
			dispersion=0.0060000001;
			magazineSlot="magazine";
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
	class GP_Glock19: GP_Glock19_Base
	{
		scope=2;
		displayName="Glock 19";
		descriptionShort="Glock 19 is a small semi-automatic pistol.";
		model="\dz\weapons\pistols\glock\Glock19.p3d";
		attachments[]=
		{
			"pistolMuzzle",
			"pistolFlashlight"
		};
		baseAttachments[]={};
		randomAttachments[]=
		{
			
			{
				"PistolSuppressor",
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
				"TLRLight",
				"",
				"",
				"",
				"",
				""
			},
			
			{
				"Mag_Glock_15Rnd",
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
		itemSize[]={3,3};
		dexterity=3.5999999;
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
								"DZ\weapons\pistols\glock\data\glock19.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\pistols\glock\data\glock19_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\pistols\glock\data\glock19_destruct.rvmat"
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
	recoil_Glock[]=
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
	recoil_Glock_prone[]=
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