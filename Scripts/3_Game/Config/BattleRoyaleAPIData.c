class BattleRoyaleAPIData extends BattleRoyaleDataBase
{
    string endpoint = BATTLEROYALE_WEB_CLIENT_ENDPOINT;  //used by clients, comes from constants
    string api_key = "no_private_key_set"; //server-only, must come from the config
    string bans_api_key = "no_api_key_set"; //server-only, must come from the config for bans to operate
    bool use_api = true; //for private servers, set this to false

    override string GetPath()
    {
        return BATTLEROYALE_SETTINGS_FOLDER + "api_settings.json";
    }
    override void Save()
    {
        JsonFileLoader<BattleRoyaleAPIData>.JsonSaveFile(GetPath(), this);
    }
    override void Load()
    {
        JsonFileLoader<BattleRoyaleAPIData>.JsonLoadFile(GetPath(), this);

        //automatically update the key in config
        if(bans_api_key == "")
        {
            bans_api_key = "no_api_key_set";
            Save();
        }
    }
}