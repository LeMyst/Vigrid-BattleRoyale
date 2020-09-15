class MatchData
{
    int start_time; //MS since server start that match starts
    int end_time; //MS since server start that match ends
    
    ref MatchWorld world;
    ref MatchPlayer winner; 
    ref array<ref MatchPlayer> deaths;
    ref array<ref MatchZone> zones;
    ref array<ref MatchAirdrop> airdrops;
    ref array<ref MatchRedzone> redzones;

    void MatchData()
    {
        deaths = new array<ref MatchPlayer>();
        zones = new array<ref MatchZone>();
        airdrops = new array<ref MatchAirdrop>();
        redzones = new array<ref MatchRedzone>();
    }

    string GetJSON()
    {
        string result = "";
        JsonSerializer m_Serializer = new JsonSerializer;
        if(!m_Serializer.WriteToString(this, false, result))
        {
            Error("Failed to parse MatchData to JSON");
            return "{}";
        }
        return result;
    }
    
    void SetEnd(int time)
    {
        end_time = time;
    }
    void SetStart(int time)
    {
        start_time = time;
    }

    void CreateWorld(string map, string game_type)
    {
        world = new MatchWorld;
        world.map = map;
        world.type = game_type;
    }
    void SetWorldStartTime(int hour, int minute)
    {
        world.hour = hour;
        world.minute = minute;
    }
    void SetWorldWeather(float rain, float fog)
    {
        world.rain = rain;
        world.fog = fog;
    }
    
    void CreateWinner(string steamid)
    {
        winner - new MatchPlayer;
        winner.steam_id = steamid;
        winner.MatchDeath = null;
    }
    
    bool ContainsDeath(string steamid)
    {
        bool result = false;
        for(int i = 0; i < deaths.Count(); i++)
        {
            if(deaths[i].steam_id == steamid)
                return true;
        }
        return result;
    }

    void CreateDeath(string steamid, vector position, int time, string killer_steamid, notnull ref array<string> killer_weapon_attachments, vector killer_position)
    {
        ref MatchPlayer player = new MatchPlayer;
        ref MatchDeath death = new MatchDeath;
        
        player.steam_id = steamid;
        player.death_data = death;

        death.killer_id = killer_steamid;
        death.killer_weapondata = killer_weapon_attachments;
        death.killer_position = killer_position;
        death.time = time;
        death.position = position;

        deaths.Insert( player );
    }

    void CreateZone(vector position, float radius, int time)
    {
        ref MatchZone zone = new MatchZone;
        zone.position = position;
        zone.radius = radius;
        zone.time = time;

        zones.Insert( zone );
    }

    void CreateAirdrop(vector position, int time)
    {
        ref MatchAirdrop airdrop = new MatchAirdrop;
        airdrop.position = position;
        airdrop.time = time;

        airdrops.Insert( airdrop );
    }

    void CreateRedzone(vector position, float radius, int time)
    {
        ref MatchRedzone zone = new MatchRedzone;
        zone.position = position;
        zone.radius = radius;
        zone.time = time;

        redzones.Insert( zone );
    }
}
//match objects
class MatchPlayer
{
    string steam_id;
    ref MatchDeath death_data; //if winner this is NULL
}
class MatchWorld
{
    string map;
    string type;
    int hour; //on start
    int minute; //on start
    float rain; //on end
    float fog; //on end
}

//timed events
class MatchRedzone
{
    int time;
    vector position;
    float radius;
}
class MatchAirdrop
{
    int time;
    vector position;
}
class MatchZone
{
    int time; //time since start of match
    vector position;
    float radius;
}
class MatchDeath
{
    string killer_id;
    ref array<string> killer_weapondata; //weapon and all it's attachments
    vector killer_position;

    int time; //time since start of match
    vector position;
}