class BattleRoyaleLootSettings extends BattleRoyaleDataBase
{
    float chance_to_spawn_building = 0.95;
    float chance_to_spawn_pile = 0.5;

    int num_mags_to_spawn_with = 3;
    int num_ammoboxs_to_spawn_with = 2;
    
    int chance_to_spawn_attachment = 0.5;


    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "loot_settings.json";
    }
    override void Save()
    {
        JsonFileLoader<BattleRoyaleLootData>.JsonSaveFile(GetPath(), this);
    }
    override void Load()
    {
        JsonFileLoader<BattleRoyaleLootData>.JsonLoadFile(GetPath(), this);
    }
}