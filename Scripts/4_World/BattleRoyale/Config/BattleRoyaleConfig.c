static const string BATTLEROYALE_FOLDER = "$profile:BattleRoyale\\";


//BattleRoyaleConfig m_config = GetBRConfig();
class BattleRoyaleConfig
{
    ref map<string, ref BattleRoyaleDataBase> m_Configs;
    ref array<string> m_ConfigNames;

    void BattleRoyaleConfig()
    {
        m_Configs = new map<string, ref BattleRoyaleDataBase>();
    }
    //if you want to add more battle royale configs, do so here.
    void Init()
    {
        m_Configs.Insert("GameData", new BattleRoyaleGameData);
        m_Configs.Insert("DebugData", new BattleRoyaleDebugData);
        m_Configs.Insert("ZoneData", new BattleRoyaleZoneData);
    }

    void Load()
    {
        if ( GetGame().IsServer() )
        {
            //load JSON data (or create it)
            if( !FileExist(BATTLEROYALE_FOLDER))
                MakeDirectory(BATTLEROYALE_FOLDER);

            //iterate over internal data in the dictionary
            foreach(BattleRoyaleDataBase config : m_Configs.GetValueArray())
            {
                string path = config.GetPath();
                if(path != "")
                {
                    if(FileExist( path ))
                        config.Load();
                    else
                        config.Save();
                }
                else
                {
                    Error("Config with invalid path in BattleRoyale Configs")
                }
            }
        }
    }

    //if a 3rd party needs to get config by string, it can do so here
    BattleRoyaleDataBase GetConfig(string key)
    {
        return m_Configs.Get(key);
    }


    BattleRoyaleGameData GetGameData()
    {
        return GetConfig("GameData");
    }
    BattleRoyaleDebugData GetDebugData()
    {
        return GetConfig("DebugData");
    }
    BattleRoyaleZoneData GetZoneData()
    {
        return GetConfig("ZoneData");
    }
}