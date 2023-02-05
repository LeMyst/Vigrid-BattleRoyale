class BattleRoyalePrepare extends BattleRoyaleState
{
    protected ref array<PlayerBase> m_PlayerList;
    protected ref array<string> a_StartingItems;
    protected vector world_center;

    private BattleRoyaleGameData m_GameSettings;

    void BattleRoyalePrepare()
    {
        m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            a_StartingItems = m_GameSettings.player_starting_items;
        }
        else
        {
            Error("GameSettings is NULL!");
            a_StartingItems = {"TrackSuitJacket_Red", "TrackSuitPants_Red", "JoggingShoes_Red"};
        }

        m_PlayerList = new array<PlayerBase>();

        string path = "CfgWorlds " + GetGame().GetWorldName();
        world_center = GetGame().ConfigGetVector(path + " centerPosition");
    }

	override void Activate()
	{
		super.Activate();

        //if(BattleRoyaleAPI.GetAPI().ShouldUseApi())
        //{
        //    //this is still like not working properly or something
        //    ServerData m_ServerData = BattleRoyaleAPI.GetAPI().GetCurrentServer();
        //    while(!m_ServerData || m_ServerData.locked == 0)
        //    {
        //        Print("Attempting to lock the server!");
        //        BattleRoyaleAPI.GetAPI().ServerSetLock(true); //Lock the server
        //        m_ServerData = BattleRoyaleAPI.GetAPI().GetServer(m_ServerData._id);
        //    }
        //}

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        m_PlayerList.InsertAll( m_Players ); //this gave an error when using m_Players

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input on all clients (we'll do this on the server in another thread)

        GetGame().GameScript.Call(this, "ProcessPlayers", NULL); //Spin up a new thread to process giving players items and teleporting them
	}

	override void Deactivate()
	{
		super.Deactivate();

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(false), true); //fade out screen
	}
    
	override bool IsComplete()
	{
	    Print(GetName() + " IsComplete!");
		return super.IsComplete();
	}

    override string GetName()
	{
		return DAYZBR_SM_PREPARE_NAME;
	}

    protected bool DeleteAllItems(PlayerBase process_player)
    {
        if (process_player.GetInventory().CountInventory() > 0)
        {
            process_player.RemoveAllItems();
        }

        return true;
    }

    protected bool AddStartItems(PlayerBase process_player)
    {
        DeleteAllItems(process_player);

        EntityAI shirt = process_player.GetInventory().CreateAttachment("TrackSuitJacket_Black");
        EntityAI pants = process_player.GetInventory().CreateAttachment("TrackSuitPants_Black");
        EntityAI shoes = process_player.GetInventory().CreateAttachment("JoggingShoes_Black");

        pants.GetInventory().CreateEntityInCargo("HuntingKnife");
        pants.GetInventory().CreateEntityInCargo("BandageDressing");
        pants.GetInventory().CreateEntityInCargo("Compass");

        return true;
    }

    protected void GiveStartingItems(PlayerBase process_player)
    {
        //drop the item out of the player's hands before we delete it
        if(process_player.GetItemInHands())
        {
            ItemBase item_inhands = process_player.GetItemInHands();
            process_player.ServerDropEntity( item_inhands );
            //GetGame().GameScript.Call(GetGame(), "ObjectDelete", item_inhands);
            GetGame().ObjectDelete( item_inhands );
        }
        DeleteAllItems(process_player);
        AddStartItems(process_player);
    }

    protected void DisableInput(PlayerBase process_player)
    {
        process_player.DisableInput(true);
    }

    protected Town GetRandomVillage()
    {
        // https://github.com/InclementDab/DayZ-Dabs-Framework/blob/production/DabsFramework/Scripts/3_Game/DabsFramework/Town/TownFlags.c
        ref array<ref Town> villages = Town.GetMapTowns(15);
        Town random_village = villages[Math.RandomInt(0,villages.Count())];
        return random_village;
    }

    protected void Teleport(PlayerBase process_player)
    {
        vector random_pos = "0 0 0";

        BattleRoyaleZone m_Zone = new BattleRoyaleZone;
        m_Zone = m_Zone.GetZone(1);

        while(true) 
        {
            float x;
            float y;
            float z;

            if (m_GameSettings.spawn_in_villages)
            {
                float village_pad = 100.0;

                Town village = GetRandomVillage();
                float village_x = village.Position[0];
                float village_z = village.Position[2];

                x = Math.RandomFloatInclusive((village_x - village_pad), (village_x + village_pad));
                z = Math.RandomFloatInclusive((village_z - village_pad), (village_z + village_pad));
                y = GetGame().SurfaceY(x, z);
            } else {
                float edge_pad = 0.1;

                x = Math.RandomFloatInclusive((world_center[0] * edge_pad), (world_center[0] * 2) - (world_center[0] * edge_pad));
                z = Math.RandomFloatInclusive((world_center[1] * edge_pad), (world_center[1] * 2) - (world_center[1] * edge_pad));
                y = GetGame().SurfaceY(x, z);
            }

            random_pos[0] = x;
            random_pos[1] = y;
            random_pos[2] = z;

            //check if random_pos is bad
            if(GetGame().SurfaceIsSea(x, z))
                continue;
            
            if(GetGame().SurfaceIsPond(x, z))
                continue;

            if(GetGame().SurfaceRoadY(x, z) != y)
                continue;

            if(!m_Zone.IsInZone(x, z))
                continue;

            //Namalsk snow biome check
            ref array<string> bad_surface_types_namalsk = {
                "nam_seaice",
                "nam_lakeice_ext"
            };
            
            string surface_type;
            GetGame().SurfaceGetType(x, z, surface_type);
            if(bad_surface_types_namalsk.Find(surface_type) != -1)
                continue;

            vector start = random_pos + Vector( 0, 5, 0 );
            vector end = random_pos;
            float radius = 2.0; 

            PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
            Object m_HitObject;
            vector m_HitPosition;
            vector m_HitNormal;
            float m_HitFraction;
            bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );

            if(m_Hit)
                continue;

            break;
        }

        //set pos
        process_player.SetPosition(random_pos);

        //TODO: make sure the retarded game engine doesn't keep the player in a swimming state ????
        //TODO: force stand up
        
        //random direction
        float dir = Math.RandomFloat(0,360); //non-inclusive, 360==0
        vector playerDir = vector.YawToVector(dir);
        process_player.SetDirection(Vector(playerDir[0], 0, playerDir[1]));
    }

    void ProcessPlayers()
    {
        Print("Starting to process players...");
        int i;
        PlayerBase process_player;

        int pCount = m_Players.Count();
        for (i = 0; i < pCount; i++) {
            process_player = m_Players[i];
            if (process_player) DisableInput(process_player);

            Sleep(100);
        }
        Print("Players are disabled");
        
        for (i = 0; i < pCount; i++) {
            process_player = m_Players[i];
            if (process_player) Teleport(process_player);

            Sleep(100);
        }
        Print("Teleported players");

        // plz fix this
        Sleep(1000);

        for (i = 0; i < pCount; i++) {
            process_player = m_Players[i];
            if (process_player) GiveStartingItems(process_player);

            Sleep(100);
        }
        Print("Gave starting items");
        Deactivate();
    }

    override void OnPlayerTick(PlayerBase player, float timeslice)
	{
		super.OnPlayerTick(player, timeslice);

		if(player.time_until_heal <= 0)
		{
			player.time_until_heal = 5;
			player.Heal();
		}
		player.time_until_heal -= timeslice;
	}

    override void AddPlayer(PlayerBase player)
	{
		if(player)
		{
			player.SetAllowDamage(false); //all players in this state are god mode
			player.Heal();
		}
		super.AddPlayer(player);
	}

    override ref array<PlayerBase> RemoveAllPlayers()
	{
		ref array<PlayerBase> players = super.RemoveAllPlayers();
		foreach(PlayerBase player : players)
		{
			player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
		}
		return players;
	}

	override void RemovePlayer(PlayerBase player)
	{
		if(player)
        {
			player.SetAllowDamage(true); //leaving debug state = disable god mode
            player.Heal();
        }
		super.RemovePlayer(player);
	}
}
