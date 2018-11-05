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
	class GP_Colt1911_Base: FNX45
	{
		scope=0;
		weight=1106;
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
			"Ammo_45ACP"
		};
		magazines[]=
		{
			"Mag_1911_7Rnd"

		};
		magazineSwitchTime=0.24;
		ejectType=1;
		recoilModifier[]={1.1, 2.1, 1.1};
		drySound[]=
		{
			"dz\sounds\weapons\firearms\colt1911\Colt1911_dry",
			0.5,
			1,
			20
		};
		reloadAction="ReloadAKM";
		reloadMagazineSound[]=
		{
			"dz\sounds\weapons\firearms\colt1911\Colt1911_reload",
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
				"Colt1911_Shot_SoundSet",
				"Colt1911_Tail_SoundSet",
				"Colt1911_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"Colt1911_silencerPro_SoundSet",
					"Colt1911_silencerTail_SoundSet",
					"Colt1911_silencerInteriorTail_SoundSet"
				}
				
			};
			begin1[]=
			{
				"dz\sounds\weapons\firearms\colt1911\Colt1911_0",
				1,
				1,
				700
			};
			begin2[]=
			{
				"dz\sounds\weapons\firearms\colt1911\Colt1911_1",
				1,
				1,
				700
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
			reloadTime=0.13;
			recoil="recoil_1911";
			recoilProne="recoil_1911_prone";
			dispersion=0.006;
			magazineSlot="magazine";
			beginSilenced_Pro[]=
			{
				"dz\sounds\weapons\firearms\colt1911\1911Silenced",
				1,
				1,
				60
			};
			beginSilenced_HomeMade[]=
			{
				"dz\sounds\weapons\firearms\colt1911\1911Silenced",
				1,
				1,
				100
			};
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
	class GP_Colt1911: GP_Colt1911_Base
	{
		scope=2;
		displayName="Colt 1911";
		descriptionShort="Semi-Automatic pistol designed in 1911 by John Browning. Since then it has been used as standard sidearm by miliary and government agencies of USA and many other countries.";
		model="\dz\weapons\pistols\1911\1911.p3d";
		attachments[]=
		{
			"pistolFlashlight",
			"pistolMuzzle"
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
				""
			},
			
			{
				"Mag_1911_7Rnd",
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
		itemSize[]={4,2};
		dexterity=3.6;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\pistols\fnx45\data\1911_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\pistols\fnx45\data\1911.rvmat"
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
								"DZ\weapons\pistols\1911\data\1911.rvmat",
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\pistols\1911\data\1911_damage.rvmat",
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\pistols\1911\data\1911_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class GP_Colt1911Engraved: GP_Colt1911_Base
	{
		scope=2;
		displayName="Colt 1911 Engraved";
		descriptionShort="Special engraved edition of 1911 pistol.";
		model="\dz\weapons\pistols\1911\1911_engraved.p3d";
		attachments[]=
		{
			"pistolFlashlight",
			"pistolMuzzle"
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
				"Mag_1911_7Rnd",
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
		itemSize[]={4,3};
		lootTag[]=
		{
			"Military_west"
		};
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\pistols\1911\data\1911_engraved_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\pistols\1911\data\1911_engraved.rvmat"
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
								"DZ\weapons\pistols\1911\data\1911_engraved.rvmat",
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\pistols\1911\data\1911_engraved_damage.rvmat",
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\pistols\1911\data\1911_engraved_destruct.rvmat"
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
	recoil_1911[]=
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
	recoil_1911_prone[]=
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
