#ifdef SERVER
class BattleRoyaleGameData: BattleRoyaleDataBase
{
    int num_zones = 6;
    int round_duration_minutes = 5; //round length in minutes

    int time_until_teleport_unlock = 10; //seconds before unlock after teleporting & preparing

    ref array<int> zone_notification_minutes = { 1, 2 };
    ref array<int> zone_notification_seconds = { 30, 10 };

    int debug_heal_tick_seconds = 5;
    int zone_damage_tick_seconds = 5;

    float zone_damage_delta = 0.1;
    bool enable_zone_damage = true;

    bool hide_players_endgame = false;

    bool show_first_zone_at_start = true;

    bool artillery_sound = true;

    ref array<string> player_starting_clothes = {
        "TrackSuitJacket_Red",
        "TrackSuitPants_Red",
        "JoggingShoes_Red"
    };

    ref array<string> player_starting_items = {
        "HuntingKnife",
        "BandageDressing",
        "Compass",
        "Battery9V",
        "Battery9V"
    };

    string mission = "BattleRoyale.ChernarusPlus";

    bool use_spectate_whitelist = true;
    bool auto_spectate_mode = true;
    ref array<string> allowed_spectate_steamid64 = {
        "123456789123456789" // Dummy SteamID64
    };

    //--- do not modify
    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "general_settings.json";
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleGameData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleGameData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
