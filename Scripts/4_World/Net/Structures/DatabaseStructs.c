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

class RegionData {
    ref array<string> regions;
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
    ref array<ref BRRawMatchMovementEvent> movements;
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
    BRRawMatchCircleEvent_Events event;
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
    BRRawMatchLootEvent_Events event;
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
    float pos;
    string shooter;
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
    BRRawMatchVehicleEvent_Events event;
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
    string map;
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
        game.map = map_name;
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
        winner.killerpos = Vector(0,0,0)


        results.Insert(winner)
    }
    void CreateDeath(string playerid, vector pos, int time, string killerid, string killerweapon, vector killerpos)
    {
        ref BRRawMatchPlayer dead = new BRRawMatchPlayer;

        dead.steamid = playerid;
        dead.killedby = killerid;
        dead.killedwith = killerweapon;
        dead.timestamp = time;
        dead.pos = pos;
        dead.killerpos = killerpos


        results.Insert(dead)
    }
    bool ContainsDeath(string playerid)
    {
        for(int i = 0; i < results.Count(); i++)
        {
            if(deaths[i].steamid == playerid)
                return true;
        }
        return false;
    }
    void ShowZone(vector position, float radius, int time)
    {
        ref BRRawMatchCircleEvent event = new BRRawMatchCircleEvent;
        event.event = BRRawMatchCircleEvent_Events.Event_ShowCircle;
        event.pos = position;
        event.radius = radius;
        event.timestamp = time;

        events.circles.Insert( event );
    }
    void LockZone(vector position, float radius, int time)
    {
        ref BRRawMatchCircleEvent event = new BRRawMatchCircleEvent;
        event.event = BRRawMatchCircleEvent_Events.Event_LockCircle;
        event.pos = position;
        event.radius = radius;
        event.timestamp = time;

        events.circles.Insert( event );
    }
    //events! --- need implementations!

   
    void KillZombie(string playerid, vector zombiepos, int time)
    {
        ref BRRawMatchZombieEvent event = new BRRawMatchZombieEvent;
        event.playerid = playerid;
        event.pos = zombiepos;
        event.timestamp = time;

        events.zombiekills.Insert( event );
    }
    void GetInVehicle(string playerid, string vehicletype, vector vehiclepos, int time)
    {
        ref BRRawMatchVehicleEvent event = new BRRawMatchVehicleEvent;
        event.event = BRRawMatchVehicleEvent_Events.Event_GetIn;
        event.playerid = playerid;
        event.vehicle = vehicletype;
        event.pos = vehiclepos;
        event.timestamp = time;

        events.vehicles.Insert( event );
    }
    void GetOutVehicle(string playerid, string vehicletype, vector vehiclepos, int time)
    {
        ref BRRawMatchVehicleEvent event = new BRRawMatchVehicleEvent;
        event.event = BRRawMatchVehicleEvent_Events.Event_GetOut;
        event.playerid = playerid;
        event.vehicle = vehicletype;
        event.pos = vehiclepos;
        event.timestamp = time;

        events.vehicles.Insert( event );
    }
    void Shoot(string playerid, vector pos, int time)
    {
        ref BRRawMatchShotEvent event = new BRRawMatchShotEvent;
        event.playerid = playerid;
        event.pos = pos;
        event.timestamp = timestamp;

        events.shots.Insert( event );
    }
    void Hit(string playerid, string shooterid, vector playerpos, int time)
    {
        ref BRRawMatchHitEvent event = new BRRawMatchHitEvent;
        event.playerid = playerid;
        event.pos = playerpos;
        event.timestamp = timestamp;
        event.shooter = shooterid;

        events.hits.Insert( event );
    }
    void Movement(string playerid, vector pos, float dir, int time)
    {
        ref BRRawMatchMovementEvent event = new BRRawMatchMovementEvent;
        event.playerid = playerid;
        event.pos = pos;
        event.direction = dir;
        event.timestamp = time;

        events.movements.Insert( event );
    }
    void LootPickedUp(string playerid, string loottype, vector pos, int time)
    {
        ref BRRawMatchLootEvent event = new BRRawMatchLootEvent;
        event.event = BRRawMatchLootEvent_Events.Event_PickUp;
        event.playerid = playerid;
        event.item = loottype;
        event.pos = pos;
        event.timestamp = time;

        events.loots.Insert( event );
    }
    void LootDropped(string playerid, string loottype, vector pos, int time)
    {
        ref BRRawMatchLootEvent event = new BRRawMatchLootEvent;
        event.event = BRRawMatchLootEvent_Events.Event_Drop;
        event.playerid = playerid;
        event.item = loottype;
        event.pos = pos;
        event.timestamp = time;

        events.loots.Insert( event );
    }
    //NOT IMPLEMENTED!!!
    void Airdrop(vector pos, int time)
    {
        ref BRRawMatchAirdropEvent event = new BRRawMatchAirdropEvent;
        event.pos = pos;
        event.timestamp = time;

        events.airdrops.Insert( event );
    }
}