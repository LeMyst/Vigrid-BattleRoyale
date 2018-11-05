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
	class GP_MKII_Base: FNX45
	{
		scope=0;
		weight=1300;
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
			"Ammo_22"
		};
		magazines[]=
		{
			"Mag_MKII_10Rnd"

		};
		magazineSwitchTime=0.2;
		ejectType=1;
		recoilModifier[]={1, 1.4, 1};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\FNX45\FNX_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\MkII\MkII_reload",
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
				"AmphibianS_Shot_SoundSet",
				"AmphibianS_Tail_SoundSet",
				"AmphibianS_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"AmphibianS_silencerHomeMade_SoundSet",
					"AmphibianS_silencerHomeMadeTail_SoundSet",
					"AmphibianS_InteriorTail_SoundSet"
				}
				
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\MkII\MkII_close_0",
				1,
				1,
				80
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\MkII\MkII_close_1",
				1,
				1,
				80
			};
			begin3[]=
			{
				"dz\sounds\weapons\firearms\MkII\MkII_close_2",
				0.80000001,
				1,
				80
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
			reloadTime=0.0799;
			recoil="recoil_mkii";
			recoilProne="recoil_mkii_prone";
			dispersion=0.006;
			magazineSlot="magazine";
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\MkII\MkII_close_2",
				0.02,
				1,
				20
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
	class GP_MKII: GP_MKII_Base
	{
		scope=2;
		displayName="Amphibia S";
		descriptionShort="A rimfire single-action semi-automatic pistol. These pistols are some of the most popular handguns made. The Amphibia S is an integrally-suppressed variant.";
		model="\dz\weapons\pistols\mkii\rugerMKII.p3d";
		attachments[]=
		{
			"suppressorImpro"
		};
		baseAttachments[]={};
		randomAttachments[]=
		{
			
			{
				"Mag_MKII_10Rnd",
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
		itemSize[]={4,2};
		dexterity=3.7;
		hiddenSelectionsTextures[]=
		{
			"DZ\weapons\pistols\mkii\data\ruger_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DZ\weapons\pistols\mkii\data\ruger_metal_1.rvmat"
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
								"DZ\weapons\pistols\mkii\data\ruger_metal_1.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\pistols\mkii\data\ruger_metal_1_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\pistols\mkii\data\ruger_metal_1_destruct.rvmat"
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
	recoil_mkii[]=
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
	recoil_mkii_prone[]=
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