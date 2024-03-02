class BattleRoyaleServerData: BattleRoyaleDataBase
{
    int port = 2302;
    int query_port = 27016;
    string ip_address = "127.0.0.1"; //--- leave this default and it'll default to the box ip address

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "server_settings.json";
    }
    override void Save()
    {
        JsonFileLoader<BattleRoyaleServerData>.JsonSaveFile(GetPath(), this);
    }
    override void Load()
    {
        JsonFileLoader<BattleRoyaleServerData>.JsonLoadFile(GetPath(), this);
        //--- first time launch, assign default (updates existing servers in place)
        if(port == 0)
        {
            port = 2302;
            Save();
        }
    }
}
