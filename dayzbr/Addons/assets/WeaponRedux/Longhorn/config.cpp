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
	class GP_LongHorn_Base: FNX45
	{
		scope=0;
		weight=1700;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		discreteDistance[]={25};
		discreteDistanceInitIndex=0;
		modelOptics="-";
		distanceZoomMin=100;
		distanceZoomMax=100;
		PPDOFProperties[]={1,0.60000002,200000,0.30000001,3,0.1};
		optics=1;
		value=0;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_308Win"
		};
		ejectType=1;
		recoilModifier[]={1, 4, 1};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\mosin9130\mosin_dry",
			0.5,
			1,
			20
		};
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\LongHorn\Longhorn_reload",
			0.80000001,
			1,
			20
		};
		reloadAction="ReloadFNX";
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
				"LongHorn_Shot_SoundSet",
				"LongHorn_Tail_SoundSet",
				"LongHorn_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"LongHorn_silencerHomeMade_SoundSet",
					"LongHorn_silencerHomeMadeTail_SoundSet",
					"LongHorn_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\LongHorn\LongHorn_close_0",
				1,
				1,
				900
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\LongHorn\LongHorn_close_1",
				1,
				1,
				900
			};
			begin3[]=
			{
				"dz\sounds\weapons\firearms\LongHorn\LongHorn_close_2",
				1,
				1,
				900
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
			recoil="recoil_longhorn";
			recoilProne="recoil_longhorn_prone";
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
	class GP_LongHorn: GP_LongHorn_Base
	{
		scope=2;
		displayName="LongHorn";
		descriptionShort="Break-action LongHorn pistol uses a trigger mechanism, designed to be strong and to make the break-action easy to open. It uses a large and strong frame.";		model="\dz\weapons\pistols\LongHorn\LongHorn.p3d";
		attachments[]=
		{
			"suppressorImpro",
			"weaponOpticsCrossbow"
		};
		baseAttachments[]=
		{
			"PistolOptic"
		};
		randomAttachments[]=
		{
			
			{
				"Crossbow_RedpointOptic",
				"PistolOptic",
				"PistolOptic",
				"PistolOptic",
				"PistolOptic",
				"PistolOptic",
				"PistolOptic",
				"PistolOptic"
			}
		};
		itemSize[]={4,3};
		dexterity=3.7;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\pistols\LongHorn\data\LongHorn_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\pistols\LongHorn\data\LongHorn.rvmat"
		};
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
								"DZ\weapons\pistols\LongHorn\data\LongHorn.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\pistols\LongHorn\data\LongHorn_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\pistols\LongHorn\data\LongHorn_destruct.rvmat"
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
	recoil_longhorn[]=
	{
		0,
		0,
		0,
		0.039999999,
		"0.036943*(1)",
		"0.0134348*(3)",
		0.079999998,
		"0.019755*(1)",
		"0.003056*(3)",
		0.090000004,
		0,
		0,
		0.14,
		"-0.003138*(1)",
		"-0.0005*(3)",
		0.079999998,
		"-0.001177*(1)",
		"-0.000188*(3)",
		0.12,
		0,
		0
	};
	recoil_longhorn_prone[]=
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
	class ProxyDE: ProxyAttachment
	{
		scope=2;
		inventorySlot="pistol";
		model="\dz\weapons\pistols\DE\DE.p3d";
	};
};