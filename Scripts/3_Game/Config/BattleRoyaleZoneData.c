class BattleRoyaleZoneData extends BattleRoyaleDataBase
{
    int shrink_type = 1; // Exp/Lin/Static/Const
    
    //constant
    float constant_scale = 0.65;

    //exponential 
    float shrink_base = 2.718281828459; // ~ e
    float shrink_exponent = 3.0;

    //ref array<float> static_sizes = { 6000, 5000, 4000, 3000, 2000, 1000, 100 }; //note, this needs as many entries as Num_Rounds in GameSettings
    ref array<float> static_sizes = { 4500, 3375, 2250, 1125, 562, 140, 35 };

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
