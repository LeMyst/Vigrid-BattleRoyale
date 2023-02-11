class BattleRoyaleZoneData extends BattleRoyaleDataBase
{
    int shrink_type = 3; // Exp/Lin/Static/Const

    //constant
    float constant_scale = 0.65;

    //exponential
    float shrink_base = 2.718281828459; // ~ e
    float shrink_exponent = 3.0;

    //ref array<float> static_sizes = { 6000, 5000, 4000, 3000, 2000, 1000, 100 }; //note, this needs as many entries as Num_Rounds in GameSettings
    //ref array<float> static_sizes = { 4500, 3375, 2250, 1125, 562, 140, 35 };
    ref array<float> static_sizes = { 35, 140, 562, 1125, 2250, 3375, 4500 };
    ref array<int> static_timers = { 75, 100, 132, 177, 236, 315, 420 };

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
