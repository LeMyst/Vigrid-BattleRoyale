#ifdef SERVER
class BattleRoyaleServerData: BattleRoyaleDataBase
{
    int port = 2302;
    int query_port = 27016;
    string ip_address = "127.0.0.1"; //--- leave this default and it'll default to the box ip address

    string webhook_jwt_token = "dummy";
    bool force_match_uuid = false;

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
