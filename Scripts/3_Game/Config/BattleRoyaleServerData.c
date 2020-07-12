class BattleRoyaleServerData extends BattleRoyaleDataBase
{
    int query_port = "27016";
    string ip_address = "127.0.0.1"; //--- leave this default and it'll default to the box ip address

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "server_settings.json";
    }
    override void Save()
    {
        JsonFileLoader<BattleRoyaleDebugData>.JsonSaveFile(GetPath(), this);
    }
    override void Load()
    {
        JsonFileLoader<BattleRoyaleDebugData>.JsonLoadFile(GetPath(), this);
    }
}