class BattleRoyaleZoneData extends BattleRoyaleDataBase
{
    int shrink_type = 1; // constant shrinkage 
    
    //constant
    float constant_scale = 0.65;

    //TODO: linear

    //exponential 
    float shrink_base = 2.718281828459; // ~ e
    float shrink_exponent = 3.0;
    


    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "zone_settings.json";
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