
//--- vehicle spawning subsystem
class BattleRoyaleVehicles
{
    protected int i_TickTime = 1000;
    protected int i_NumVehicles = 1000;
    protected float f_DespawnRadius = 500;

    protected ref array<ref BattleRoyaleCachedVehicle> m_Vehicles;
    protected ref ScriptCallQueue m_CallQueue;


    protected ref BattleRoyaleVehicleData m_SettingsData;

    protected bool b_Enabled;
    protected bool b_IsReady;
    protected bool b_IsBusy;
    protected bool b_IsInitializing;

    void BattleRoyaleVehicles()
    {
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        i_TickTime = m_GameSettings.vehicle_ticktime_ms;
        i_NumVehicles = m_GameSettings.num_vehicles;
        f_DespawnRadius = m_GameSettings.vehicle_spawn_radius;

        Print("BattleRoyaleVehicles::Constructor()");
        Print(i_TickTime);
        Print(i_NumVehicles);
        Print(f_DespawnRadius);

        m_Vehicles = new array<ref BattleRoyaleCachedVehicle>();
        m_CallQueue = new ScriptCallQueue;

        m_SettingsData = BattleRoyaleVehicleData.Cast( BattleRoyaleConfig.GetConfig().GetConfig("VehicleData") );

        m_CallQueue.CallLater( this.OnTick, i_TickTime, true); //tick every second
        
        b_IsBusy = false;
        b_IsReady = false;
        b_Enabled = false;
        b_IsInitializing = false;
    }

    void Update(float delta)
    {
        if(b_Enabled && b_IsReady)
        {
            m_CallQueue.Tick(delta);
        }
    }

    


    void Start()
    {
        Print("Starting vehicle subsystem...");
        b_Enabled = true;
        Preinit();
    }
    void Stop()
    {
        b_Enabled = false;
    }

    void Preinit()
    {
        if(!b_IsReady && !b_IsInitializing)
        {
            Print("Initializing Vehicle Subsystem...");
            GetGame().GameScript.Call(this, "Init", NULL);
        }
        else
        {
            Print("Preinit called on vehicle subsystem when not necessary!");
        }
        
    }
    void Init()
    {
        Print("Vehicle Subsystem:");
        Print(i_NumVehicles);
        b_IsInitializing = true;

        int i;
        ref BattleRoyaleVehicleDataSerialized vehicle_data;

        float max_roll = 0;
        //max roll is only used if settings are valid
        if(m_SettingsData && m_SettingsData.m_VehicleData && m_SettingsData.m_VehicleData.Count() > 0)
        {
            for(i = 0; i < m_SettingsData.m_VehicleData.Count(); i++)
            {
                vehicle_data = m_SettingsData.m_VehicleData[i];
                max_roll += vehicle_data.Weight;
            }
        }

        for(i = 0; i < i_NumVehicles; i++)
        {
            
            vector position = DAYZBR_DEBUG_CENTER; //this is only a temp value so we'll just stuck the broken debug zone center position
        
            Print("Finding valid location...");
            string path = "CfgWorlds " + GetGame().GetWorldName();
            vector temp = GetGame().ConfigGetVector(path + " centerPosition");

            float world_width = temp[0] * 2;
            float world_height = temp[1] * 2;
            int iterations = 0;
            while(true)
            {
                iterations++;
                float x = Math.RandomFloat(0, world_width);
                float z = Math.RandomFloat(0, world_height);
                float y = GetGame().SurfaceY(x, z);
                position = Vector( x, y, z );

                if(!IsSafeSpawnPos(position))
                    continue;

                
                
                Print(position);


                break;
            }

            //--- defaults in case no config data is found
            string vehicle_class = "Sedan_02";
            ref array<string> vehicle_parts = {"CarBattery", "CarRadiator", "SparkPlug", "Sedan_02_Wheel", "Sedan_02_Wheel", "Sedan_02_Wheel", "Sedan_02_Wheel"};
           
            if(m_SettingsData && m_SettingsData.m_VehicleData && m_SettingsData.m_VehicleData.Count() > 0)
            {
                if(max_roll > 0)
                {
                    float roll = Math.RandomFloat(0, max_roll);
                    float val = 0;
                    for(int j = 0; j < m_SettingsData.m_VehicleData.Count(); j++)
                    {
                        vehicle_data = m_SettingsData.m_VehicleData[j];
                        val += vehicle_data.Weight;
                        
                        //this is the vehicle we rolled
                        if(val > roll)
                        {
                            vehicle_class = vehicle_data.VehicleName;
                            vehicle_parts = vehicle_data.Parts;
                            Print("Adding " + vehicle_class + " to vehicle subsystem!");
                            break;
                        }
                    }
                }
                else
                {
                    Error("Total Vehicle Weights are set to 0, could not roll for random vehicles!");
                }
                
            }
            else
            {
                Error("Failed to access Vehicle Data Settings!");
            }
            
            //create server marker for this vehicle
            //ExpansionMarkerModule.Cast( GetModuleManager().GetModule( ExpansionMarkerModule ) ).CreateServerMarker( vehicle_class, "Car", position, ARGB(255, 0, 255, 0), false );

            ref BattleRoyaleCachedVehicle cached_vehicle = new BattleRoyaleCachedVehicle( vehicle_class, vehicle_parts, position );
            m_Vehicles.Insert( cached_vehicle );
        }
        Print("Vehicle Subsystem Ready!");
        b_IsReady = true;
    }

    void OnTick()
    {
        if(!b_IsBusy)
        {
            b_IsBusy = true;
            GetGame().GameScript.Call(this, "ProcessVehicles", NULL); //spin up thread
        }
    }


    protected bool IsSafeSpawnPos(vector pos)
    {
        float x = pos[0];
        float z = pos[2];

        if(GetGame().SurfaceIsSea(x, z))
            return false;
        if(GetGame().SurfaceIsPond(x, z))
            return false;

        string surface_type;
        GetGame().SurfaceGetType(x, z, surface_type);

        string cfgSurfacePath = "CfgSurfaces " + surface_type;
        int is_interior = GetGame().ConfigGetInt(cfgSurfacePath + " interior");

         //Invalid if GetInt(CfgSurfaces >> surface_type >> interior) == 1
        if(is_interior == 1)
            return false;

        float friction = GetGame().ConfigGetFloat(cfgSurfacePath + " friction");

        //Invalid if GetFloat(... friction) < 0.94     
        if(friction < DAYZBR_VS_MIN_SURFACE_FRICTION)
            return false;


        vector start = pos;
        vector end = pos + Vector( 0, 1, 0 );
        float radius = 2.0; 
        PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
        Object m_HitObject;
        vector m_HitPosition;
        vector m_HitNormal;
        float m_HitFraction;

        //check if safe from object collisions using same raycast as players tp
        bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );
        if(m_Hit)
            return false;

       
        Print("Found safe vehicle spawn position using IsSafeSpawnPos");
        return true;
    }

    void ProcessVehicles()
    {
        BattleRoyaleServer server_module = BattleRoyaleServer.Cast(GetBR());
        BattleRoyaleState current_state = server_module.GetCurrentState();
        ref array<PlayerBase> spawnable_players = new array<PlayerBase>();
        //duplicate all players into a new array
        spawnable_players.InsertAll(current_state.GetPlayers());

        for(int i = 0; i < m_Vehicles.Count(); i++)
        {
            ref BattleRoyaleCachedVehicle m_Vehicle = m_Vehicles[i];

            if(m_Vehicle)
            {
                vector position = m_Vehicle.GetPosition();
               
                bool b_IsPlayerNear = false;

                for(int j = 0; j < spawnable_players.Count(); j++)
                {
                    PlayerBase player = spawnable_players[j];
                    if(vector.Distance(player.GetPosition(), position) < f_DespawnRadius)
                    {
                        b_IsPlayerNear = true;
                        break;
                    }
                }            



                if(m_Vehicle.IsSpawned() && !b_IsPlayerNear)
                {

                    if(!m_Vehicle.Despawn())
                    {
                        Error("Failed to despawn vehicle!");
                    }
                }
                else if(!m_Vehicle.IsSpawned() && b_IsPlayerNear)
                {
                    if(!m_Vehicle.Spawn())
                    {
                        Error("Failed to spawn vehicle!");
                    }
                }
            }
        }

        b_IsBusy = false; //complete!
    }

}

class BattleRoyaleCachedVehicle
{
    protected Object game_object;
    protected string vehicle_name;
    protected vector position;
    protected vector direction;
    protected ref array<string> vehicle_parts;

    void BattleRoyaleCachedVehicle(string classname, ref array<string> parts, vector pos)
    {
        vehicle_name = classname;
        vehicle_parts = parts;
        position = pos;
    }
    private void FillCar( Car car, CarFluid fluid )
	{
		float cap = car.GetFluidCapacity( fluid );
		car.Fill( fluid, cap );
	}

    vector GetPosition()
    {
        if(IsSpawned())
        {
            return game_object.GetPosition();
        }
        else
        {
            return position;
        }
    }

    bool IsSpawned()
    {
        return (game_object != NULL);
    }
    bool Spawn()
    {
        Print("Spawning Vehicle " + vehicle_name);

        Object obj = GetGame().CreateObject( vehicle_name, position, false, false, true );

        EntityAI ent;
        if ( !Class.CastTo( ent, obj ) )
        {
            if(obj != NULL)
            {
                GetGame().ObjectDelete( obj );
            }
			return false;
        }
        for ( int j = 0; j < vehicle_parts.Count(); j++ )
		{
			ent.GetInventory().CreateInInventory( vehicle_parts[j] );
		}

        Car car;
		if ( Class.CastTo( car, ent ) )
		{
			FillCar( car, CarFluid.FUEL );
			FillCar( car, CarFluid.OIL );
			FillCar( car, CarFluid.BRAKE );
			FillCar( car, CarFluid.COOLANT );
		}
        else
        {
            
            /*
            if(obj != NULL)
            {
                GetGame().ObjectDelete( obj );
            }
            return false;
            */
        }
        

        ent.SetPosition( position );
        
        if(direction)
        {
		    ent.SetDirection( direction );
        }
        else
        {
            float random_dir = Math.RandomFloat(0, 360);
            vector dir = vector.YawToVector(random_dir);
            ent.SetDirection( Vector( dir[0], 0, dir[1]) );
        }
        
        vector tmItem[4];
		ent.GetTransform( tmItem );

        //ent.PlaceOnSurfaceRotated( tmItem, position, 0, 0, 0, true ); // TOOD: this needs done to support vehicles in tight spaces
        tmItem[3] = position + "0 5 0";
		ent.SetTransform( tmItem );

        game_object = obj;

        return true;
    }
    bool Despawn()
    {
        if(game_object)
        {
            Print("Despawning Vehicle " + vehicle_name)
            position = GetPosition();
            direction = game_object.GetDirection();
            //TODO: cache vehicle inventory contents
            GetGame().ObjectDelete( game_object ); 
            game_object = NULL;
            return true;
        }
        return false;
    }
}