class BattleRoyaleZoneData extends BattleRoyaleDataBase
{
    float constant_scale = 0.75;


    override string GetPath()
    {
        return BATTLEROYALE_FOLDER + "zone_settings.json";
    }
    override void Save()
    {
        JsonFileLoader<BattleRoyaleZoneData>.JsonSaveFile(GetPath(), this);
    }
    override void Load()
    {
        JsonFileLoader<BattleRoyaleZoneData>.JsonLoadFile(GetPath(), this);
    }
}