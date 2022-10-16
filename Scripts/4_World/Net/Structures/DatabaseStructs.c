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
    ref array<string> ips;
}

class RegionData {
    ref array<string> regions;
}

class LeaderboardPlayer {
    string _id;
    string playerid;
    string steamid;
    string description;
    float rating;
    ref LeaderboardPlayerPentagon pentagon;
   // ref map<int, float> ratingtime; //--- dayz doesn't support floats in map keytype?! "Map's key type can be only int, string and enum"
    float averageplace;
    int totalkills;
    int totalwins;
    int totalmatches;
    int totallooted;
    int totalhits;
    float totaldistanceonfoot;
    float totaldistanceinvehicle;
    int totaltimealive;
    int maxpossibletimealive;
    int averagetimealive;
    ref array<ref LeaderboardPlayerMatch> matches;
}

class LeaderboardPlayerPentagon {
    float looting;
    float fighting;
    float walking;
    float surviving;
    float driving;
}

class LeaderboardPlayerMatch {
    string matchid;
    string matchname;
    int placement;
    int kills;
    float postmatchrating;
    float deltarating;
    int timestamp;
}

class BRRawMatch {
    ref BRRawMatchWeather weather;  //DONE!
    ref BRRawMatchGame game;  //DONE!
    ref array<ref BRRawMatchPlayer> results;  //DONE! //index 0 is the first dead, last index is the winner
    ref BRRawMatchEvents events;
}

class BRRawMatchEvents {
    ref array<ref BRRawMatchZombieEvent> zombiekills;
    ref array<ref BRRawMatchVehicleEvent> vehicles;
    ref array<ref BRRawMatchShotEvent> shots;
    ref array<ref BRRawMatchHitEvent> hits;
    ref array<ref BRRawMatchMovementEvent> movements;//DONE!
    ref array<ref BRRawMatchLootEvent> loots;
    ref array<ref BRRawMatchCircleEvent> circles; //DONE!
    ref array<ref BRRawMatchAirdropEvent> airdrops;
}

class BRRawMatchAirdropEvent {
    vector pos;
    int timestamp;
}

enum BRRawMatchCircleEvent_Events {
    Event_ShowCircle = 0,
    Event_LockCircle
}

class BRRawMatchCircleEvent {
    vector pos;
    float radius;
    int brevent;
    int timestamp;
}

enum BRRawMatchLootEvent_Events {
    Event_PickUp = 0,
    Event_Drop
}

class BRRawMatchLootEvent {
    string playerid;
    string item;
    vector pos;
    int brevent;
    int timestamp;
}

class BRRawMatchMovementEvent {
    string playerid;
    vector pos;
    float direction;
    int timestamp;
}

class BRRawMatchHitEvent {
    string playerid;
    vector pos;
    string shooterid;
    int timestamp;
}

class BRRawMatchShotEvent {
    string playerid;
    vector pos;
    int timestamp;
}

enum BRRawMatchVehicleEvent_Events {
    Event_GetIn = 0,
    Event_GetOut
}

class BRRawMatchVehicleEvent {
    string playerid;
    string vehicle;
    vector pos;
    int brevent;
    int timestamp;
}

class BRRawMatchZombieEvent {
    string playerid;
    vector pos;
    int timestamp;
}

class BRRawMatchPlayer {
    string steamid;
    string killedby;
    string killedwith;
    int timestamp;
    vector pos;
    vector killerpos;
}

class BRRawMatchGame {
    string name;
    string mapname;
    string gametype;
    int start;
    int end;
}

class BRRawMatchWeather {
    float fog;
    float rain;
    int hour;
    int minute;
}

//---- below are the wrapped objects that make managing these structures WAY easier
class BRPlayerData extends BRPlayer {}

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

//TODO update!
class MatchData extends BRRawMatch {
    void MatchData(string match_name, string map_name, string game_type)
    {
        weather = new BRRawMatchWeather;
        game = new BRRawMatchGame;
        results = new array<ref BRRawMatchPlayer>;
        events = new BRRawMatchEvents;

        //--- initialize our game constants
        game.name = match_name;
        game.mapname = map_name;
        game.gametype = game_type;


        events.zombiekills = new array<ref BRRawMatchZombieEvent>;
        events.vehicles = new array<ref BRRawMatchVehicleEvent>;
        events.shots = new array<ref BRRawMatchShotEvent>;
        events.hits = new array<ref BRRawMatchHitEvent>;
        events.movements = new array<ref BRRawMatchMovementEvent>;
        events.loots = new array<ref BRRawMatchLootEvent>;
        events.circles = new array<ref BRRawMatchCircleEvent>;
        events.airdrops = new array<ref BRRawMatchAirdropEvent>;

    }
    //set match end timestamp (all timestamps are aquired with GetGame().GetTime() )
    void SetEnd(int time)
    {
        game.end = time;
    }
    void SetStart(int time) 
    {
        game.start = time;
    }
    void SetStartTime(int hour, int minute)
    {
        weather.hour = hour;
        weather.minute = minute;
    }
    void SetEndWeather(float rain, float fog)
    {
        weather.rain = rain;
        weather.fog = fog;
    }
    void CreateWinner(string playerid, vector pos, int time) 
    {
        ref BRRawMatchPlayer winner = new BRRawMatchPlayer;

        winner.steamid = playerid;
        winner.killedby = "";
        winner.killedwith = "";
        winner.timestamp = time;
        winner.pos = pos;
        winner.killerpos = Vector(0,0,0);
        results.Insert(winner);
    }
    void CreateDeath(string playerid, vector pos, int time, string killerid, string killerweapon, vector killerpos)
    {
        ref BRRawMatchPlayer dead = new BRRawMatchPlayer;

        dead.steamid = playerid;
        dead.killedby = killerid;
        dead.killedwith = killerweapon;
        dead.timestamp = time;
        dead.pos = pos;
        dead.killerpos = killerpos;
        results.Insert(dead);
    }
    bool ContainsDeath(string playerid)
    {
        for(int i = 0; i < results.Count(); i++)
        {
            if(results[i].steamid == playerid)
                return true;
        }
        return false;
    }
    void ShowZone(vector position, float radius, int time)
    {
        ref BRRawMatchCircleEvent brevent = new BRRawMatchCircleEvent;
        brevent.brevent = BRRawMatchCircleEvent_Events.Event_ShowCircle;
        brevent.pos = position;
        brevent.radius = radius;
        brevent.timestamp = time;

        events.circles.Insert( brevent );
    }
    void LockZone(vector position, float radius, int time)
    {
        ref BRRawMatchCircleEvent brevent = new BRRawMatchCircleEvent;
        brevent.brevent = BRRawMatchCircleEvent_Events.Event_LockCircle;
        brevent.pos = position;
        brevent.radius = radius;
        brevent.timestamp = time;

        events.circles.Insert( brevent );
    }
    void Movement(string playerid, vector pos, float dir, int time)
    {
        ref BRRawMatchMovementEvent brevent = new BRRawMatchMovementEvent;
        brevent.playerid = playerid;
        brevent.pos = pos;
        brevent.direction = dir;
        brevent.timestamp = time;

        events.movements.Insert( brevent );
    }
    void GetInVehicle(string playerid, string vehicletype, vector vehiclepos, int time)
    {
        ref BRRawMatchVehicleEvent brevent = new BRRawMatchVehicleEvent;
        brevent.brevent = BRRawMatchVehicleEvent_Events.Event_GetIn;
        brevent.playerid = playerid;
        brevent.vehicle = vehicletype;
        brevent.pos = vehiclepos;
        brevent.timestamp = time;

        events.vehicles.Insert( brevent );
    }
    void GetOutVehicle(string playerid, string vehicletype, vector vehiclepos, int time)
    {
        ref BRRawMatchVehicleEvent brevent = new BRRawMatchVehicleEvent;
        brevent.brevent = BRRawMatchVehicleEvent_Events.Event_GetOut;
        brevent.playerid = playerid;
        brevent.vehicle = vehicletype;
        brevent.pos = vehiclepos;
        brevent.timestamp = time;

        events.vehicles.Insert( brevent );
    }
    void KillZombie(string playerid, vector zombiepos, int time)
    {
        ref BRRawMatchZombieEvent brevent = new BRRawMatchZombieEvent;
        brevent.playerid = playerid;
        brevent.pos = zombiepos;
        brevent.timestamp = time;

        events.zombiekills.Insert( brevent );
    }
    void Shoot(string playerid, vector pos, int time)
    {
        ref BRRawMatchShotEvent brevent = new BRRawMatchShotEvent;
        brevent.playerid = playerid;
        brevent.pos = pos;
        brevent.timestamp = time;

        events.shots.Insert( brevent );
    }
    void Hit(string playerid, string shooterid, vector playerpos, int time)
    {
        ref BRRawMatchHitEvent brevent = new BRRawMatchHitEvent;
        brevent.playerid = playerid;
        brevent.pos = playerpos;
        brevent.timestamp = time;
        brevent.shooterid = shooterid;

        events.hits.Insert( brevent );
    }
    void LootPickedUp(string playerid, string loottype, vector pos, int time)
    {
        ref BRRawMatchLootEvent brevent = new BRRawMatchLootEvent;
        brevent.brevent = BRRawMatchLootEvent_Events.Event_PickUp;
        brevent.playerid = playerid;
        brevent.item = loottype;
        brevent.pos = pos;
        brevent.timestamp = time;

        events.loots.Insert( brevent );
    }
    void LootDropped(string playerid, string loottype, vector pos, int time)
    {
        ref BRRawMatchLootEvent brevent = new BRRawMatchLootEvent;
        brevent.brevent = BRRawMatchLootEvent_Events.Event_Drop;
        brevent.playerid = playerid;
        brevent.item = loottype;
        brevent.pos = pos;
        brevent.timestamp = time;

        events.loots.Insert( brevent );
    }
    
    //NOT IMPLEMENTED!!!
    void Airdrop(vector pos, int time)
    {
        ref BRRawMatchAirdropEvent brevent = new BRRawMatchAirdropEvent;
        brevent.pos = pos;
        brevent.timestamp = time;

        events.airdrops.Insert( brevent );
    }
}
