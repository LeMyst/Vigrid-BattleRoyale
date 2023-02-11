class BattleRoyaleConfig
{
    static ref BattleRoyaleConfig m_Instance;
    ref map<string, ref BattleRoyaleDataBase> m_Configs;
    ref array<string> m_ConfigNames;
    bool b_HasLoaded;

    void BattleRoyaleConfig()
    {
        b_HasLoaded = false;
        m_Configs = new map<string, ref BattleRoyaleDataBase>();
        Init();
    }

    static BattleRoyaleConfig GetConfig()
    {
        if(!m_Instance)
        {
            m_Instance = new BattleRoyaleConfig;
            m_Instance.Load();
        }
        return m_Instance;
    }

    //if you want to add more battle royale configs, do so here.
    void Init()
    {
        Print("Initializing Settings...");

        BattleRoyaleGameData p_GameData = new BattleRoyaleGameData;
        if(p_GameData)
            m_Configs.Insert("GameData",p_GameData);
        else
            Error("BattleRoyaleGameData Setting Constructor Returned NULL");

        BattleRoyaleDebugData p_DebugData = new BattleRoyaleDebugData;
        if(p_DebugData)
            m_Configs.Insert("DebugData", p_DebugData);
        else
            Error("BattleRoyaleDebugData Setting Constructor Returned NULL");

        BattleRoyaleZoneData p_ZoneData = new BattleRoyaleZoneData;
        if(p_ZoneData)
            m_Configs.Insert("ZoneData", p_ZoneData);
        else
            Error("BattleRoyaleZoneData Setting Constructor Returned NULL");

        //--- adding a new config? copy below
        BattleRoyaleServerData p_ServerData = new BattleRoyaleServerData;
        if(p_ServerData)
            m_Configs.Insert("ServerData", p_ServerData);
        else
            Error("BattleRoyaleServerData Setting Constructor Returned NULL");

        //--- paste it here
        BattleRoyaleLootSettings p_LootData = new BattleRoyaleLootSettings;
        if(p_LootData)
            m_Configs.Insert("LootData", p_LootData);
        else
            Error("BattleRoyaleLootSettings Setting Constructor Returned NULL");

        BattleRoyaleVehicleData p_VehicleData = new BattleRoyaleVehicleData;
        if(p_VehicleData)
            m_Configs.Insert("VehicleData", p_VehicleData);
        else
            Error("BattleRoyaleVehicleData Setting Constructor Returned NULL");
    }

    void Load()
    {
        if ( GetGame().IsServer() && !b_HasLoaded )
        {
            b_HasLoaded = true;
            //load JSON data (or create it)
            if( !FileExist(BATTLEROYALE_SETTINGS_FOLDER))
            {
                Print("Creating BattleRoyale Settings Folder");
                MakeDirectory(BATTLEROYALE_SETTINGS_FOLDER);
            }

            if(!m_Configs)
            {
                Error("FAILED TO LOAD CONFIG DATA");
                return;
            }
            //iterate over internal data in the dictionary
            for(int i = 0; i < m_Configs.Count(); i++)
            {
                string key = m_Configs.GetKey(i);
                BattleRoyaleDataBase config = m_Configs.GetElement(i);
                if(config)
                {
                    string path = config.GetPath();
                    if(path != "")
                    {
                        if(FileExist( path ))
                        {
                            Print("Loading Config: " + path);
                            config.Load();
                            if(BATTLEROYALE_SOLO_GAME)
                            {
                                //config.Save(); //re-save (if there are new config values that need added to the json file)
                            }
                        }
                        else
                        {
                            Print("Saving Config: " + path);
                            if(BATTLEROYALE_SOLO_GAME)
                            {
                                //config.Save();
                            }
                        }
                    }
                    else
                    {
                        Error("Config with invalid path in BattleRoyale Configs");
                    }
                }
                else
                {
                    Error("NULL CONFIG `" + key + "` IN CONFIG MAP");
                }
            }
        }
    }

    //if a 3rd party needs to get config by string, it can do so here
    BattleRoyaleDataBase GetConfig(string key)
    {
        if(!b_HasLoaded)
        {
            Error("Requesting Config Data from Unloaded Config?");
            Load();
        }
        return m_Configs.Get(key);
    }

    BattleRoyaleServerData GetServerData()
    {
        Print("Accessing Server Data Config...");
        return BattleRoyaleServerData.Cast( GetConfig("ServerData") );
    }

    BattleRoyaleGameData GetGameData()
    {
        Print("Accessing Game Data Config...");
        return BattleRoyaleGameData.Cast( GetConfig("GameData") );
    }

    BattleRoyaleDebugData GetDebugData()
    {
        Print("Accessing Debug Data Config...");
        return BattleRoyaleDebugData.Cast( GetConfig("DebugData") );
    }

    BattleRoyaleZoneData GetZoneData()
    {
        Print("Accessing Zone Data Config...");
        return BattleRoyaleZoneData.Cast( GetConfig("ZoneData") );
    }
}
