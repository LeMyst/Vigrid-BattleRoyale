#ifdef SERVER
class BattleRoyaleConfig
{
	static ref BattleRoyaleConfig m_Instance;
	ref map<string, ref BattleRoyaleDataBase> m_Configs;
	//ref array<string> m_ConfigNames;
	bool b_HasLoaded;  // set only once Load() has actually completed - see Load()
	bool b_IsLoading;  // re-entrancy guard while Load() is in flight

	void BattleRoyaleConfig()
	{
		b_HasLoaded = false;
		b_IsLoading = false;
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
		BattleRoyaleUtils.Trace("Initializing Settings...");

		BattleRoyaleLobbyData p_DebugData = new BattleRoyaleLobbyData;
		if(p_DebugData)
			m_Configs.Insert("DebugData", p_DebugData);
		else
			Error("BattleRoyaleLobbyData Setting Constructor Returned NULL");

		BattleRoyaleGameData p_GameData = new BattleRoyaleGameData;
		if(p_GameData)
			m_Configs.Insert("GameData",p_GameData);
		else
			Error("BattleRoyaleGameData Setting Constructor Returned NULL");

		BattleRoyaleServerData p_ServerData = new BattleRoyaleServerData;
		if(p_ServerData)
			m_Configs.Insert("ServerData", p_ServerData);
		else
			Error("BattleRoyaleServerData Setting Constructor Returned NULL");

		BattleRoyaleSpawnsData p_SpawnsData = new BattleRoyaleSpawnsData;
		if(p_SpawnsData)
			m_Configs.Insert("SpawnsData", p_SpawnsData);
		else
			Error("BattleRoyaleSpawnsData Setting Constructor Returned NULL");

		BattleRoyalePOIsData p_POIsData = new BattleRoyalePOIsData;
		if(p_POIsData)
			m_Configs.Insert("POIsData", p_POIsData);
		else
			Error("BattleRoyalePOIsData Setting Constructor Returned NULL");

		BattleRoyaleZoneData p_ZoneData = new BattleRoyaleZoneData;
		if(p_ZoneData)
			m_Configs.Insert("ZoneData", p_ZoneData);
		else
			Error("BattleRoyaleZoneData Setting Constructor Returned NULL");

		//--- adding a new config? copy below
	}

	void Load()
	{
		//--- b_HasLoaded means "a load COMPLETED", not "a load was attempted". It used to be set as the
		//--- very first statement here, which made the !b_HasLoaded guard in GetConfig(string) unable to
		//--- detect an aborted load - the early-out below left the singleton flagged loaded with zero
		//--- configs and every accessor silently returning NULL. b_IsLoading takes over the re-entrancy
		//--- job that flag-first assignment was doing.
		if ( b_HasLoaded || b_IsLoading )
			return;

		b_IsLoading = true;

		//load JSON data (or create it)
		if( !FileExist(BATTLEROYALE_SETTINGS_FOLDER) )
		{
			BattleRoyaleUtils.Trace("Creating BattleRoyale Settings Folder");
			MakeDirectory(BATTLEROYALE_SETTINGS_FOLDER);
		}

		if( !FileExist(BATTLEROYALE_SETTINGS_MISSION_FOLDER) )
		{
			Print("Creating BattleRoyale Mission Settings Folder");
			MakeDirectory(BATTLEROYALE_SETTINGS_MISSION_FOLDER);
		}

		if(!m_Configs)
		{
			//--- Kept Warn + return rather than the old fatal Error(): m_Configs is built in the ctor
			//--- and never cleared, so this is a can't-happen guard, and the useful behaviour if it ever
			//--- did happen is to leave b_HasLoaded false and let GetConfig(string) report it - which a
			//--- VM-halting Error() would make unreachable, since the return below would never run.
			BattleRoyaleUtils.Warn("FAILED TO LOAD CONFIG DATA - m_Configs is NULL");
			b_IsLoading = false;
			return;  //--- deliberately leaves b_HasLoaded false so the failure stays detectable
		}

		//iterate over internal data in the dictionary
		for(int i = 0; i < m_Configs.Count(); i++)
		{
			string key = m_Configs.GetKey(i);
			BattleRoyaleDataBase config = m_Configs.GetElement(i);
			if(config)
			{
				string path = config.GetProfilePath();
				if(path != "")
				{
					if(FileExist( path ))
					{
						BattleRoyaleUtils.Trace("Loading Config: " + path);
						config.Load();
						config.Save(); //re-save (if there are new config values that need added to the json file)
					}
					else
					{
						BattleRoyaleUtils.Trace("Creating Config: " + path);
						config.Save();
					}
				}
				else
				{
					//--- Warn, not Error: the global Error() is Error2() (endebug.c:90), which raises a
					//--- VM exception and stops the script VM - it would take the server down over one
					//--- misdeclared data class instead of skipping it and loading the other five.
					BattleRoyaleUtils.Warn("Config with invalid path in BattleRoyale Configs");
				}

				string mission_path = config.GetMissionPath();
				if(mission_path != "")
				{
					if(FileExist( mission_path ))
					{
						BattleRoyaleUtils.Trace("Loading Mission Config: " + mission_path);
						config.LoadMission();
					}
				}
			}
			else
			{
				//--- Same reasoning as above - skip the bad entry, keep the rest of the configs.
				BattleRoyaleUtils.Warn("NULL CONFIG `" + key + "` IN CONFIG MAP");
			}
		}

		b_IsLoading = false;
		b_HasLoaded = true;
	}

	//if a 3rd party needs to get config by string, it can do so here
	BattleRoyaleDataBase GetConfig(string key)
	{
		if(!b_HasLoaded)
		{
			//--- Warn, not Error: this branch exists to RECOVER by loading, and the global Error() halts
			//--- the VM (endebug.c:90 -> Error2), so the Load() below would never have been reached.
			BattleRoyaleUtils.Warn("Requesting Config (" + key + ") Data from Unloaded Config?");
			Load();
		}

		return m_Configs.Get(key);
	}

	BattleRoyaleLobbyData GetDebugData()
	{
		BattleRoyaleUtils.Trace("Accessing Debug Data Config...");

		return BattleRoyaleLobbyData.Cast( GetConfig("DebugData") );
	}

	BattleRoyaleGameData GetGameData()
	{
		BattleRoyaleUtils.Trace("Accessing Game Data Config...");

		return BattleRoyaleGameData.Cast( GetConfig("GameData") );
	}

	BattleRoyaleServerData GetServerData()
	{
		BattleRoyaleUtils.Trace("Accessing Server Data Config...");

		return BattleRoyaleServerData.Cast( GetConfig("ServerData") );
	}

	BattleRoyaleSpawnsData GetSpawnsData()
	{
		BattleRoyaleUtils.Trace("Accessing Spawns Data Config...");

		return BattleRoyaleSpawnsData.Cast( GetConfig("SpawnsData") );
	}

	BattleRoyalePOIsData GetPOIsData()
	{
		BattleRoyaleUtils.Trace("Accessing POIs Data Config...");

		return BattleRoyalePOIsData.Cast( GetConfig("POIsData") );
	}

	BattleRoyaleZoneData GetZoneData()
	{
		BattleRoyaleUtils.Trace("Accessing Zone Data Config...");

		return BattleRoyaleZoneData.Cast( GetConfig("ZoneData") );
	}
};
