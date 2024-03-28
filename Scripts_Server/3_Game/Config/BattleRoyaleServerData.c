#ifdef SERVER
class BattleRoyaleServerData: BattleRoyaleDataBase
{
    int port = 2302;
    int query_port = 27016;
    string ip_address = "127.0.0.1"; //--- leave this default and it'll default to the box ip address

    string webhook_server_id = "00000000-0000-4000-a000-000000000000";
    string webhook_server_secret = "dummy";

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "server_settings.json";
    }

    override void Save()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleServerData>.SaveFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }

    override void Load()
    {
    	string errorMessage;
        if (!JsonFileLoader<BattleRoyaleServerData>.LoadFile(GetPath(), this, errorMessage))
			ErrorEx(errorMessage);
    }
};
#endif
