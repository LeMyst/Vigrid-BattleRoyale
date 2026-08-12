#ifdef SERVER
// Voice policy settings, split out from general_settings.json because BattleRoyaleVoice.c is a
// self-contained subsystem (party-only voice while frozen, the speaking-players panel) with its
// own settings surface - same reasoning as leaderboard_settings.json getting its own file.
class BattleRoyaleVoiceData: BattleRoyaleDataBase
{
	int version = 1;  // Config version

    // While players are frozen (countdown, spawn selection, prepare) a player hears only their own
    // party. Solo players hear nobody. The lobby is deliberately not covered - it stays open voice.
    bool party_only_voice = true;  // restrict voice to party members while players are frozen
    bool show_speaking_players = true;  // show the on-screen list of who is currently speaking
    bool speaking_list_during_match = true;  // keep that list up after the match has started

    // How far a voice carries, per voice level, in metres. Used ONLY to decide who appears in the
    // speaking list - it does not change what players actually hear, which the engine decides and
    // does not expose. While party-only voice is active these are ignored, because membership then
    // answers audibility exactly. Tune these if the list names people you cannot hear, or misses
    // people you can.
    //
    // These are the community-reported vanilla ranges rather than measured ones - the engine does
    // not expose the real values anywhere in script, so they cannot be verified from code. Erring
    // small is the safer direction: a radius that is too large names people you cannot actually
    // hear, which is worse than occasionally missing someone at the edge of earshot.
    float voice_radius_whisper = 7;
    float voice_radius_talk = 25;
    float voice_radius_shout = 45;

    override string GetProfilePath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "voice_settings.json";
    }

    override string GetMissionPath()
    {
        return BATTLEROYALE_SETTINGS_MISSION_FOLDER + "voice_settings.json";
    }

	override void Load()
	{
		string errorMessage;
		// Load from profile folder
		if (FileExist(GetProfilePath()))
		{
			if (!JsonFileLoader<BattleRoyaleVoiceData>.LoadFile(GetProfilePath(), this, errorMessage))
				ErrorEx(errorMessage);
		}

		// Run the upgrade function here to avoid overrides from mission folder
		Upgrade();
	}

	override void LoadMission()
	{
		string errorMessage;
		// Override from mission folder
		if (FileExist(GetMissionPath()))
		{
			if (!JsonFileLoader<BattleRoyaleVoiceData>.LoadFile(GetMissionPath(), this, errorMessage))
				ErrorEx(errorMessage);
		}
	}

	override void Save()
	{
		string errorMessage;
		if (!JsonFileLoader<BattleRoyaleVoiceData>.SaveFile(GetProfilePath(), this, errorMessage))
			ErrorEx(errorMessage);
	}

	override void Upgrade()
	{
		// Nothing to migrate yet - this class started at v1 with the fields above already present.
		// Simply ADDING a field needs no bump (JsonFileLoader leaves the initialiser in place for a
		// key an older file doesn't have). Only a changed meaning for an existing value needs one.
	}
};
