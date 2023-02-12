class BattleRoyaleVehicleData extends BattleRoyaleDataBase
{
    ref array<ref BattleRoyaleVehicleDataSerialized> m_VehicleData;

    void BattleRoyaleVehicleData()
    {
        m_VehicleData = new array<ref BattleRoyaleVehicleDataSerialized>();
    }

    override string GetPath()
    {
        return BATTLEROYALE_VEHICLES_FOLDER;
    }

    override void Save()
    {
        if( !FileExist(BATTLEROYALE_VEHICLES_FOLDER))
        {
            Print("Creating BattleRoyale Vehicles Folder");
            MakeDirectory(BATTLEROYALE_VEHICLES_FOLDER);

            //Load Defaults
            Defaults();
        }

        for(int i = 0; i < m_VehicleData.Count(); i++)
        {
            m_VehicleData[i].Save();
        }
    }

    override void Load()
    {
        m_VehicleData.Clear();

        array< string > files = FindFilesInLocation( BATTLEROYALE_VEHICLES_FOLDER );

        for ( int i = 0; i < files.Count(); i++ )
        {
            string fileName;
            int pos = files[i].IndexOf(".");
            if ( pos > -1 )
            {
                fileName = files[i];
                ref BattleRoyaleVehicleDataSerialized vehicle_data = new BattleRoyaleVehicleDataSerialized();
                vehicle_data.Load(fileName);
                m_VehicleData.Insert(vehicle_data);
            }
        }
    }

    void Defaults()
    {
        //--- sedan default
        ref BattleRoyaleVehicleDataSerialized sedan = new BattleRoyaleVehicleDataSerialized();
        sedan.m_FileName = "Sedan.json";
        sedan.VehicleName = "Sedan_02";
        sedan.Parts = new array<string>();
        sedan.Parts.Insert("Sedan_02_Hood");
        sedan.Parts.Insert("Sedan_02_Trunk");
        sedan.Parts.Insert("Sedan_02_Door_1_1");
        sedan.Parts.Insert("Sedan_02_Door_1_2");
        sedan.Parts.Insert("Sedan_02_Door_2_1");
        sedan.Parts.Insert("Sedan_02_Door_2_2");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("Sedan_02_Wheel");
        sedan.Parts.Insert("CarBattery");
        sedan.Parts.Insert("CarRadiator");
        sedan.Parts.Insert("SparkPlug");
        sedan.Weight = 1.0;
        m_VehicleData.Insert(sedan);
    }
}

class BattleRoyaleVehicleDataSerialized : Managed
{
    [NonSerialized()]
    string m_FileName;

    float Weight;
    string VehicleName;
    ref array<string> Parts;

    void Save()
    {
        JsonFileLoader<BattleRoyaleVehicleDataSerialized>.JsonSaveFile(BATTLEROYALE_VEHICLES_FOLDER + m_FileName, this);
    }

    void Load(string filename)
    {
        JsonFileLoader<BattleRoyaleVehicleDataSerialized>.JsonLoadFile(BATTLEROYALE_VEHICLES_FOLDER + filename , this);
        this.m_FileName = filename;
        if(this.Weight == 0)
        {
            //backwards compatibility for vehicles
            this.Weight = 1;
            Save();
        }
    }
}
