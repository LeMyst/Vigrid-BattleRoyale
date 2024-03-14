#ifdef SERVER
class BattleRoyaleSpawnsData: BattleRoyaleDataBase
{
    bool spawn_in_villages = true;
    bool spawn_in_first_zone = true;
    int extra_spawn_radius = 250;

    ref array<string> avoid_city_spawn = {
        "Camp_Shkolnik",
        "Hill_Zelenayagora",
        "Settlement_Kumyrna",
        "Ruin_Voron",
        "Settlement_Skalisty",
        "Settlement_Novoselki",
        "Settlement_Dubovo",
        "Settlement_Vysotovo"
    };

    ref array<ref BattleRoyaleOverrideSpawnPosition> override_spawn_positions;

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "spawns_settings.json";
    }

    override void Save()
    {
        JsonFileLoader<BattleRoyaleSpawnsData>.JsonSaveFile(GetPath(), this);
    }

    override void Load()
    {
        JsonFileLoader<BattleRoyaleSpawnsData>.JsonLoadFile(GetPath(), this);
    }
};

class BattleRoyaleOverrideSpawnPosition
{
    string city_name;
    vector new_position;
};
#endif
