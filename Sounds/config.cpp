class CfgPatches
{
	class BattleRoyale_Sounds
	{
		requiredAddons[]=
		{
			"DZ_Sounds_Effects"
		};
	};
};

class CfgSoundShaders
{
	class BattleRoyale_Music_Namalsk_SoundShader
	{
		samples[] = {{"\DayZBR-Mod\Sounds\Music\christmas_menu",1}};
		volume = 0.35794576;
	};
    class BattleRoyale_Apocalypse_SoundShader
	{
		samples[] = {{"\DayZBR-Mod\Sounds\Music\apocalypse",1}};
		volume = 0.70794576;
	};
};

class CfgSoundSets
{
	class BattleRoyale_Music_SoundSet
	{
		soundShaders[] = {"BattleRoyale_Apocalypse_SoundShader"};
		volumeFactor = 1;
		frequencyFactor = 1;
		spatial = 0;
	};
};
