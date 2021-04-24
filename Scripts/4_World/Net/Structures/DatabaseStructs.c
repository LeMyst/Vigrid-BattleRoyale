//--- db objects
class BRServer {
    string _id;
    string name;
    string connection;
    string query_port;
    string version;
    int last_started;
    ref array<string> matches;
    string region;
    bool locked;
}
class BRPlayer {
    string _id;
    string name;
    string steam_id;
    ref array<string> match_ids;
    int rating;
    ref array<string> ips;
    ref array<string> shop_purchases;
}

class BRMatch {
    string _id;
    int start_time;
    int end_time;
    ref BRMatchWorld world;
    ref BRMatchPlayer winner;
    ref array<ref BRMatchPlayer> deaths;
    ref array<ref BRMatchZone> zones;
    ref array<ref BRMatchAirdrop> airdrops;
    ref array<ref BRMatchRedzone> redzones;
    string server_id;
    int timestamp;
}



class RegionData {
    ref array<string> regions;
}
class BRMatchWorld {
    string map_name;
    string game_type;
    int hour;
    int minute;
    float rain;
    float fog;
}
class BRMatchPlayer {
    string steam_id;
    ref BRMatchDeath death_data;
}
class BRMatchDeath {
    string killer_id;
    ref array<string> killer_weapondata;
    vector killer_position;
    int time;
    vector position;
}
class BRMatchZone {
    vector position;
    float radius;
    int time;
}
class BRMatchAirdrop {
    vector position;
    int time;
}
class BRMatchRedzone {
    vector position;
    float radius;
    int time;
}



class PlayerData extends BRPlayer {}
class ServerData extends BRServer {
    string GetIP()
	{
		TStringArray parts = new TStringArray;
		connection.Split(":",parts);
		
		return parts.Get(0);
	}
	int GetPort()
	{
		TStringArray parts = new TStringArray;
		connection.Split(":",parts);

		return parts.Get(1).ToInt();
	}
	bool CanConnect()
	{
		return !locked;
	}
	bool IsMatchingVersion()
	{
		return (version == BATTLEROYALE_VERSION);
	}
}
class MatchData extends BRMatch {
    void MatchData()
    {
        deaths = new array<ref BRMatchPlayer>();
        zones = new array<ref BRMatchZone>();
        airdrops = new array<ref BRMatchAirdrop>();
        redzones = new array<ref BRMatchRedzone>();
    }
    void SetEnd(int time)
    {
        end_time = time;
        Print("MatchData: Setting end time");
        Print(time);
    }
    void SetStart(int time)
    {
        start_time = time;
        Print("MatchData: Setting start time");
        Print(time);
    }
    void CreateWorld(string map_name, string game_type)
    {
        world = new BRMatchWorld;
        world.map_name = map_name;
        world.game_type = game_type;
        Print("MatchData: Setting world `" + map_name + "`:`" + game_type + "`");
    }
    void SetWorldStartTime(int hour, int minute)
    {
        world.hour = hour;
        world.minute = minute;
        Print("MatchData: Setting world start time");
        Print(hour);
        Print(minute);
    }
    void SetWorldWeather(float rain, float fog)
    {
        world.rain = rain;
        world.fog = fog;
        Print("MatchData: Setting world weather");
        Print(rain);
        Print(fog);
    }
    void CreateWinner(string steamid)
    {
        winner = new BRMatchPlayer;
        winner.steam_id = steamid;
        winner.death_data = null;
        Print("MatchData: Setting winner `" + steamid + "`");
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
        ref BRMatchPlayer player = new BRMatchPlayer;
        ref BRMatchDeath death = new BRMatchDeath;
        
        player.steam_id = steamid;
        player.death_data = death;

        death.killer_id = killer_steamid;
        death.killer_weapondata = killer_weapon_attachments;
        death.killer_position = killer_position;
        death.time = time;
        death.position = position;

        deaths.Insert( player );
        Print("MatchData: Inserting death");
        Print(player);
        Print(death);
    }
    void CreateZone(vector position, float radius, int time)
    {
        ref BRMatchZone zone = new BRMatchZone;
        zone.position = position;
        zone.radius = radius;
        zone.time = time;

        zones.Insert( zone );
        Print("MatchData: Inserting zone");
        Print(zone);
    }
    void CreateAirdrop(vector position, int time)
    {
        ref BRMatchAirdrop airdrop = new BRMatchAirdrop;
        airdrop.position = position;
        airdrop.time = time;

        airdrops.Insert( airdrop );
    }
    void CreateRedzone(vector position, float radius, int time)
    {
        ref BRMatchRedzone zone = new BRMatchRedzone;
        zone.position = position;
        zone.radius = radius;
        zone.time = time;

        redzones.Insert( zone );
    }
}