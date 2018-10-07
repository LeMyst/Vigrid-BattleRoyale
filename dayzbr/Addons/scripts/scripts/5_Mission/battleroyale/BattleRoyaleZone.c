class BattleRoyaleZone
{
	ref ScriptCallQueue zone_CallQueue;
	ref BattleRoyaleRound br_round;
	
	vector current_center;
	vector new_center;
	float current_size;
	float new_size;
	
	int number_of_shrinks;
	
	bool isZoning;
	
	
	
	
	void BattleRoyaleZone(BattleRoyaleRound round)
	{
		zone_CallQueue = new ScriptCallQueue();
		isZoning = true;
		number_of_shrinks = 0;
		new_size = 0;
		new_center = "0 0 0";
		current_size = 0;
		current_center = "0 0 0";
		br_round = round;
		
		MarkerSetup();
	}
	
	//Get config values (used for initialization)
	vector GetCenter()
	{
		//Get play area center from config
		return br_round.br_game.m_BattleRoyaleData.cherno_center;
	}
	float GetMaxSize()
	{
		//Get play area size from config
		return br_round.br_game.m_BattleRoyaleData.play_area_size;
	}
	float GetShrinkCoefficient()
	{
		return br_round.br_game.m_BattleRoyaleData.shrink_coefficient;
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

	float GetNewZoneSize()
	{
		switch(br_round.br_game.m_BattleRoyaleData.shrink_type)
		{
			case 1: // exponential
				// code for wolfram alpha: plot (r/-(e^3))*(e^((3/m)*x)+(-(e^3))) from x=0 to 30, r=500, m=30
				float minutes = Math.Ceil(br_round.br_game.m_BattleRoyaleData.zone_lock_time / 60) * number_of_shrinks; // x

				float base = br_round.br_game.m_BattleRoyaleData.shrink_base; // default 2.718281828459 ~ e
				float exponent = br_round.br_game.m_BattleRoyaleData.shrink_exponent; // default 3
				float max_playtime = br_round.br_game.m_BattleRoyaleData.shrink_max_playtime; // default m = 30
				float play_area_size = br_round.br_game.m_BattleRoyaleData.play_area_size; // default r = 500

				float yoffset = -1.0 * Math.Pow(base, exponent);
				float zonesizefactor = play_area_size / yoffset;
				float shrinkexponent = (exponent / max_playtime) * minutes;
				float shrinkfactor = Math.Pow(base, shrinkexponent) + yoffset;

				return zonesizefactor * shrinkfactor;

			case 2: // linear
				// code for wolfram alpha: plot -(r/m)*x+r from x=0 to 30, r=500, m=30
				float minutes = Math.Ceil(br_round.br_game.m_BattleRoyaleData.zone_lock_time / 60) * number_of_shrinks; // x
				float gradient = -1.0 * (br_round.br_game.m_BattleRoyaleData.play_area_size / br_round.br_game.m_BattleRoyaleData.shrink_max_playtime);

				return gradient * minutes + br_round.br_game.m_BattleRoyaleData.play_area_size;

			default: // shrink by constant factor each tick
				return GetCurrentSize() * GetShrinkCoefficient();
		}
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
		
		zone_CallQueue.CallLater(this.Shrink_Zone, br_round.br_game.m_BattleRoyaleData.start_shrink_zone*1000,false);
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
		int zone_lock_minutes = Math.Ceil(br_round.br_game.m_BattleRoyaleData.zone_lock_time / 60);

		string sTime = zone_lock_minutes.ToString();

		if ( zone_lock_minutes == 1 )
		{
			sTime = sTime + " MINUTE.";
		} else {
			sTime = sTime + " MINUTES.";
		}
		
		SendMessageAll("THE NEW ZONE HAS APPEARED. IT WILL LOCK IN LESS THAN " + sTime);
		number_of_shrinks++; //this will be 1 on the first shrink call (helpful for max shrinks and dynamic shrinks in the future)
		
		//Calculate new size on lock
		new_size = GetNewZoneSize();

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
		
		Print("==== ZONE LOGIC - SRHINK DONE ====");
		Print(GetCurrentCenter());
		Print(new_center);
		Print(GetCurrentSize());
		Print(new_size);
		Print("==================================");
		
		HandleMarkers();
		
		//Queue up the lock
		zone_CallQueue.CallLater(this.Lock_Zone, br_round.br_game.m_BattleRoyaleData.zone_lock_time * 1000, false);
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
		zone_CallQueue.CallLater(this.Shrink_Zone, 120*1000,false); //in 2 minutes, call next shrink
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
