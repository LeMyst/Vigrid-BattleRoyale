
//--- vehicle spawning subsystem
class BattleRoyaleVehicles
{
    //TODO: connect these with a settings file
    protected int i_TickTime = 1000;
    protected int i_NumVehicles = 1;
    protected float f_DespawnRadius = 10;

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
        for(int i = 0; i < i_NumVehicles; i++)
        {
            
            vector position = "14829.2 72.3148 14572.3";
            
            //TODO: remove this IF statement when no longer debugging vheicles
            if(i > 0)
            {
                Print("Finding valid location...");
                string path = "CfgWorlds " + GetGame().GetWorldName();
                vector temp = GetGame().ConfigGetVector(path + " centerPosition");

                float world_width = temp[0] * 2;
                float world_height = temp[1] * 2;
                while(true)
                {
                    float x = Math.RandomFloat(0, world_width);
                    float z = Math.RandomFloat(0, world_height);
                    if(GetGame().SurfaceIsSea(x, z))
                        continue;
                    if(GetGame().SurfaceIsPond(x, z))
                        continue;
                    float y_surf = GetGame().SurfaceY(x, z);
                    float y = GetGame().SurfaceRoadY(x, z);
                    if(y_surf == y)
                        continue; //can't be on road if equal
                    
                    //use Raycast to check for roadway collisions
                    vector start = Vector(x, y, z);
                    vector end = Vector(x, y_surf, z);
                    PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.ROADWAY;
                    Object m_HitObject;
                    vector m_HitPosition;
                    vector m_HitNormal;
                    float m_HitFraction;

                    bool is_road = DayZPhysics.SphereCastBullet( start, end, 2.0, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );
                    if(is_road)
                    {
                        Print("Found safe vehicle spawn position");
                        Print(start);
                        position = start;
                        break;
                    }
                }

            }
            //--- defaults in case no config data is found
            string vehicle_class = "Sedan_02";
            ref array<string> vehicle_parts = {"CarBattery", "CarRadiator", "SparkPlug", "Sedan_02_Wheel", "Sedan_02_Wheel", "Sedan_02_Wheel", "Sedan_02_Wheel"};
           
            if(m_SettingsData && m_SettingsData.m_VehicleData && m_SettingsData.m_VehicleData.Count() > 0)
            {
                int index = Math.RandomInt(0, m_SettingsData.m_VehicleData.Count());
                ref BattleRoyaleVehicleDataSerialized vehicle_data = m_SettingsData.m_VehicleData[index];
                vehicle_class = vehicle_data.VehicleName;
                vehicle_parts = vehicle_data.Parts;
                Print("Adding " + vehicle_class + " to vehicle subsystem!");
            }
            else
            {
                Error("Failed to access Vehicle Data Settings!");
            }
            
            

            ref BattleRoyaleCachedVehicle Test_Vehicle = new BattleRoyaleCachedVehicle( vehicle_class, vehicle_parts, position );
            m_Vehicles.Insert(Test_Vehicle);
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

    void ProcessVehicles()
    {
        for(int i = 0; i < m_Vehicles.Count(); i++)
        {
            ref BattleRoyaleCachedVehicle m_Vehicle = m_Vehicles[i];

            if(m_Vehicle)
            {
                vector position = m_Vehicle.GetPosition();
                vector end = position + Vector(0, 1, 0); //more efficient to have a short cast
                PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.CHARACTER|PhxInteractionLayers.CHARACTER_NO_GRAVITY;
                Object m_HitObject;
                vector m_HitPosition;
                vector m_HitNormal;
                float m_HitFraction;

                bool b_IsPlayerNear = DayZPhysics.SphereCastBullet( position, end, f_DespawnRadius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

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
        Print("Spawning " + vehicle_name);

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
            if(obj != NULL)
            {
                GetGame().ObjectDelete( obj );
            }
            return false;
        }
        

        ent.SetPosition( position );
        
        if(direction)
        {
		    ent.SetDirection( direction );
        }
        else
        {
            //TODO: random direction!
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
            Print("Despawning: " + vehicle_name)
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