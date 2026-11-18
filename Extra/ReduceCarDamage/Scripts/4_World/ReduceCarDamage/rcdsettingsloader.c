static string ReduceCarDamageDIR = "$profile:ReduceCarDamage";
static string ReduceCarDamagePATH = ReduceCarDamageDIR + "\\config.json";

class ReduceCarDamageSettings
{
    private static bool initialized = false;
    private static ref ReduceCarDamageSettings settings;

	//Default Values
    float dmgModifier = 0.8;
	float plyrdmgModifier = 0.8;
	float mindmg = 1350;
	bool subtractmindmg = true;
	bool logcrashes = false;

    static ref ReduceCarDamageSettings Get()
    {
        if (initialized)
        {
            return settings;
        }

        // Changed for Vigrid-BattleRoyale (2026-11-18): was `ref ReduceCarDamageSettings data = new ref
        // ReduceCarDamageSettings;`. DayZ 1.30 stops compiling the World module on it ("Unexpected
        // statement 'ref'") unless -newErrorsAreWarnings=1 is set. A local holds its object anyway,
        // and `settings` below keeps it.
        ReduceCarDamageSettings data = new ReduceCarDamageSettings;
		
        if (FileExist(ReduceCarDamagePATH)) //If config file exsit load the file
        {
            JsonFileLoader<ReduceCarDamageSettings>.JsonLoadFile(ReduceCarDamagePATH, data);
            initialized = true;
        }
        else //If config file doesn't exsit create the file
        {
			
            if (GetGame().IsServer() || !GetGame().IsMultiplayer())
            {
                MakeDirectory(ReduceCarDamageDIR);
                JsonFileLoader<ReduceCarDamageSettings>.JsonSaveFile(ReduceCarDamagePATH, data);
            }
        }

        settings = data;

        return settings;
    }
}