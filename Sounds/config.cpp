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
		samples[] = {{"\BattleRoyale\Sounds\Music\christmas_menu",1}};
		volume = 0.35794576;
	};
    class BattleRoyale_Music_1_SoundShader
	{
		samples[] = {{"\BattleRoyale\Sounds\Music\apocalypse",1}};
		volume = 0.70794576;
	};
};
class CfgSoundSets
{
	class BattleRoyale_Music_Namalsk_SoundSet
	{
		soundShaders[] = {"BattleRoyale_Music_1_SoundShader"};
		volumeFactor = 1;
		frequencyFactor = 1;
		spatial = 0;
	};
};