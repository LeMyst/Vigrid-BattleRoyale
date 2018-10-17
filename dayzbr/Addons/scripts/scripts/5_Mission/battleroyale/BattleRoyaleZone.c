static string BattleRoyaleZoneFolderSaveLocation = "$profile:/BRZones";

class BattleRoyaleZoneData
{
	// default data
	string zone_name = "cherno";
	vector center = Vector(6497.66, 6.01245, 2519.26);
}

class BattleRoyaleZoneManager
{
	ref StaticBRData m_BattleRoyaleData;
	ref array<ref BattleRoyaleZone> ZoneList;

	void BattleRoyaleZoneManager(StaticBRData staticdata)
	{
		m_BattleRoyaleData = staticdata;
		ZoneList = new array<ref BattleRoyaleZone>();

		if(!FileExist(BattleRoyaleZoneFolderSaveLocation))
			MakeDirectory(BattleRoyaleZoneFolderSaveLocation);

		string fileName;
		FileAttr fileAttr;
		FindFileHandle zones = FindFile(BattleRoyaleZoneFolderSaveLocation + "/*.json", fileName, fileAttr, 0);

		ref BattleRoyaleZoneData first_zone_data;
		first_zone_data = new BattleRoyaleZoneData();

		if(!zones)
		{
			Print("BR default zone doesn't exist, creating file!");
			string defaultfilepath = BattleRoyaleZoneFolderSaveLocation + "/default.json";
			Print(defaultfilepath);

			if(!FileExist(defaultfilepath))
			{
				JsonFileLoader<BattleRoyaleZoneData>.JsonSaveFile(defaultfilepath, first_zone_data);
			}

			zones = FindFile(BattleRoyaleZoneFolderSaveLocation + "/*.json", fileName, fileAttr, 0);
			if(!zones)
			{
				Print("ERROR: FAILED TO GET AT LEAST THE DEFAULT BR ZONE EVEN IF WE JUST SHOULD HAVE CREATED IT...");
				return;
			}
		}

		Print(BattleRoyaleZoneFolderSaveLocation + "/" + fileName);
		JsonFileLoader<BattleRoyaleZoneData>.JsonLoadFile(BattleRoyaleZoneFolderSaveLocation + "/" + fileName, first_zone_data);
		Print(first_zone_data.zone_name);
		Print(first_zone_data.center);

		ZoneList.Insert(new BattleRoyaleZone(staticdata, first_zone_data));

		while(FindNextFile(zones, fileName, fileAttr))
		{
			ref BattleRoyaleZoneData new_zone_data = new BattleRoyaleZoneData();
			Print("FOUND ANOTHER FILE: " + BattleRoyaleZoneFolderSaveLocation + "/" + fileName);
			JsonFileLoader<BattleRoyaleZoneData>.JsonLoadFile(BattleRoyaleZoneFolderSaveLocation + "/" + fileName, new_zone_data);
			Print(new_zone_data.zone_name);
			Print(new_zone_data.center);

			ZoneList.Insert(new BattleRoyaleZone(m_BattleRoyaleData, new_zone_data));
		}

		// print out the names again just for testing purpose
		for(int i = 0; i < ZoneList.Count(); i++)
		{
			ref	BattleRoyaleZone zone = ZoneList.Get(i);
			zone.Init();
			Print("INITIALIZED ZONE: " + zone.GetZoneName());
		}
	}

	ref BattleRoyaleZone getRandomZoneFromPool()
	{
		return ZoneList.Get(Math.RandomInt(0,ZoneList.Count()));
	}
}

class BattleRoyaleZone
{
	ref ScriptCallQueue zone_CallQueue;
	ref StaticBRData m_BattleRoyaleData;
	ref BattleRoyaleZoneData m_BattleRoyaleZoneData;

	ref array<Object> map_Buildings;

	vector current_center;
	vector new_center;
	float current_size;
	float new_size;

	int number_of_shrinks;

	bool isZoning;

	void BattleRoyaleZone(StaticBRData staticdata, BattleRoyaleZoneData zonedata)
	{
		zone_CallQueue = new ScriptCallQueue();
		map_Buildings = new array<Object>();

		isZoning = true;
		number_of_shrinks = 0;
		new_size = 0;
		new_center = "0 0 0";
		current_size = 0;
		current_center = "0 0 0";
		m_BattleRoyaleData = staticdata;
		m_BattleRoyaleZoneData = zonedata;

		MarkerSetup();
	}

	void Init()
	{
		ref array<Object> allObjects = new array<Object>();
		ref array<CargoBase> proxies = new array<CargoBase>();
		GetGame().GetObjectsAtPosition(GetCenter(), GetMaxSize(), allObjects, proxies);
		for(int i = 0; i < allObjects.Count(); i++)
		{
			Object obj = allObjects.Get(i);
			if(obj.IsBuilding())
			{
				obj.SetHealth(obj.GetMaxHealth()); //heal building to max
				map_Buildings.Insert(obj);
			}
		}
	}

	//Get config values (used for initialization)
	vector GetCenter()
	{
		//Get play area center from config
		return m_BattleRoyaleZoneData.center;
	}
	float GetMaxSize()
	{
		//Get play area size from config
		return m_BattleRoyaleData.play_area_size;
	}
	float GetShrinkCoefficient()
	{
		return m_BattleRoyaleData.shrink_coefficient;
	}


	//Get current values (used for  gameplay)
	vector GetCurrentCenter()
	{
		if(current_size == 0)
		{
			return GetCenter();
		}
		else
		{
			return current_center;
		}
	}
	float GetCurrentSize()
	{
		if(current_size == 0)
		{
			return GetMaxSize();
		}
		else
		{
			return current_size;
		}
	}

	string GetZoneName()
	{
		return m_BattleRoyaleZoneData.zone_name;
	}

	float GetNewZoneSize()
	{
		// since dayz does not want the same variable to be defined twice, we declare it now since it will be used anyway
		float seconds = (m_BattleRoyaleData.zone_lock_time + m_BattleRoyaleData.shrink_zone_every) * number_of_shrinks; // x

		if(m_BattleRoyaleDatainclude_start_shrink_zone_in_roundtime > 0)
		{
			seconds = seconds + m_BattleRoyaleData.start_shrink_zone;
		}

		float max_playtime = m_BattleRoyaleData.shrink_max_playtime * 60; // default m = 30

		Print("MINUTES UNTIL END " + (m_BattleRoyaleData.shrink_max_playtime - Math.Ceil(seconds / 60));

		switch(m_BattleRoyaleData.shrink_type)
		{
			case 1: // exponential
				// code for wolfram alpha: plot (r/-(e^3))*(e^((3/m)*x)+(-(e^3))) from x=0 to 30, r=500, m=30
				float base = m_BattleRoyaleData.shrink_base; // default 2.718281828459 ~ e
				float exponent = m_BattleRoyaleData.shrink_exponent; // default 3
				float play_area_size = m_BattleRoyaleData.play_area_size; // default r = 500

				float yoffset = -1.0 * Math.Pow(base, exponent);
				float zonesizefactor = play_area_size / yoffset;
				float shrinkexponent = (exponent / max_playtime) * seconds;
				float shrinkfactor = Math.Pow(base, shrinkexponent) + yoffset;

				return zonesizefactor * shrinkfactor;

			case 2: // linear
				// code for wolfram alpha: plot -(r/m)*x+r from x=0 to 30, r=500, m=30
				float gradient = -1.0 * (m_BattleRoyaleData.play_area_size / max_playtime);

				return gradient * seconds + m_BattleRoyaleData.play_area_size;

			default: // shrink by constant factor each tick
				return GetCurrentSize() * GetShrinkCoefficient();
		}

		// we are forced to return something, even if we never get here
		return 0.0;
	}

	void OnUpdate(float ticktime)
	{
		zone_CallQueue.Tick(ticktime);
		MarkerUpdate();
	}

	void StartZoning()
	{
		//Reset the zone locations
		this.current_size = GetMaxSize();
		this.current_center = GetCenter();
		this.number_of_shrinks = 0;

		zone_CallQueue.CallLater(this.Shrink_Zone, m_BattleRoyaleData.start_shrink_zone * 1000, false);
		isZoning = true;
	}
	void StopZoning()
	{
		zone_CallQueue.Remove(this.Lock_Zone);
		zone_CallQueue.Remove(this.Shrink_Zone);

		DeleteMarkers();

		isZoning = false;
	}

	void Shrink_Zone()
	{
		int zone_lock_minutes = Math.Ceil(m_BattleRoyaleData.zone_lock_time / 60);

		string sTime = zone_lock_minutes.ToString();

		if ( zone_lock_minutes == 1 )
		{
			sTime = sTime + " MINUTE.";
		} else {
			sTime = sTime + " MINUTES.";
		}

		SendMessageAll("THE NEW ZONE HAS APPEARED. IT WILL LOCK IN LESS THAN " + sTime);

		//Calculate new size on lock
		new_size = GetNewZoneSize();

		number_of_shrinks++; //this will be 1 on the first shrink call (helpful for max shrinks and dynamic shrinks in the future)

		if ( new_size < 18 )
		{
			new_size = 18;
		}

		//Calculate new center on lock
		float distance = Math.RandomFloatInclusive(0,GetCurrentSize() - new_size);
		float oldX = GetCurrentCenter()[0];
		float oldZ = GetCurrentCenter()[2];

		float moveDir = Math.RandomFloat(0,360) * Math.DEG2RAD;
		float dX = distance * Math.Sin(moveDir);
		float dZ = distance * Math.Cos(moveDir);
		float newX = oldX + dX;
		float newZ = oldZ + dZ;
		float newY = GetGame().SurfaceY(newX,newZ);

		new_center = Vector(newX,newY,newZ);

		Print("==== ZONE LOGIC - SHRINK DONE ====");
		Print(GetCurrentCenter());
		Print(new_center);
		Print(GetCurrentSize());
		Print(new_size);
		Print("==================================");

		HandleMarkers();

		//Queue up the lock
		zone_CallQueue.CallLater(this.Lock_Zone, m_BattleRoyaleData.zone_lock_time * 1000, false);
	}
	void Lock_Zone()
	{
		SendMessageAll("THE ZONE HAS BEEN LOCKED IN.");

		current_center = new_center;
		current_size = new_size;

		Print("==== ZONE LOGIC - ZONE LOCKED ====");
		Print(current_center);
		Print(current_size);
		Print("==================================");

		//Queue up next shrink
		zone_CallQueue.CallLater(this.Shrink_Zone, m_BattleRoyaleData.shrink_zone_every * 1000, false); //in 2 minutes, call next shrink
	}

	/*

		Note: The below code will need to be moved to the client. Zone rendering on client side can improve network

	*/




	//NOTE: This is here because i plan to move all this code out of here eventually
	bool isUpdating;
	bool spawnMarkers;
	bool moveMarkers;
	int master_index;
	int total_markers;
	ref array<Object> Zone_Markers;
	void MarkerSetup()
	{
		total_markers = 200; //TODO: check source for this value
		Zone_Markers = new array<Object>();
		master_index = 0;
		moveMarkers = false;
		spawnMarkers = false;
		isUpdating = false;
	}


	void spawnTick()
	{
		for(int i = 0; i < 5; i++)
		{
			master_index--;

			float centerX = new_center[0];
			float centerZ = new_center[2];
			float distance = new_size;

			float deltaAngle = 360.0 / total_markers;

			float angle = deltaAngle * master_index;
			float rads = angle * Math.DEG2RAD;

			float dX = Math.Sin(rads) * distance;
			float dZ = Math.Cos(rads) * distance;

			float x = centerX + dX;
			float z = centerZ + dZ;
			float y = GetGame().SurfaceY(x,z);

			vector objPos = Vector(x,y,z);
			vector objDir = vector.Direction(new_center,objPos).Normalized();

			Object marker = GetGame().CreateObject("Land_Br_Wall",objPos);
			marker.SetDirection(objDir);

			Zone_Markers.Insert(marker);

			if(master_index == 0)
			{
				isUpdating = false;
				spawnMarkers = false;
				break;
			}
		}
	}
	void updateTick()
	{
		for(int j = 0; j < 5;j++)
		{
			master_index--;

			Object marker = Zone_Markers.Get(master_index);

			float centerX = new_center[0];
			float centerZ = new_center[2];
			float distance = new_size;

			float deltaAngle = 360.0 / total_markers;

			float angle = deltaAngle * master_index;
			float rads = angle * Math.DEG2RAD;

			float dX = Math.Sin(rads) * distance;
			float dZ = Math.Cos(rads) * distance;
			float x = centerX + dX;
			float z = centerZ + dZ;
			float y = GetGame().SurfaceY(x,z) + 1.5; //y correction

			vector objPos = Vector(x,y,z);
			vector objDir = vector.Direction(new_center,objPos).Normalized();

			marker.SetPosition(objPos);
			marker.SetDirection(objDir);

			if(master_index == 0)
			{
				isUpdating = false;
				moveMarkers = false;
				break;
			}
		}
	}
	void MarkerUpdate()
	{
		if(isUpdating)
		{
			if(spawnMarkers)
			{
				spawnTick();
			}
			else
			{
				updateTick();
			}
		}
	}
	void HandleMarkers()
	{

		if(Zone_Markers.Count() == 0)
		{
			master_index = total_markers;
			spawnMarkers = true;
		}
		else
		{
			master_index = Zone_Markers.Count();
			moveMarkers = true;
		}

		isUpdating = true;
	}


	void DeleteMarkers()
	{
		for(int i = 0; i < Zone_Markers.Count();i++)
		{
			Object marker = Zone_Markers.Get(i);
			GetGame().ObjectDelete(marker);
		}
		Zone_Markers.Clear();
	}
}
