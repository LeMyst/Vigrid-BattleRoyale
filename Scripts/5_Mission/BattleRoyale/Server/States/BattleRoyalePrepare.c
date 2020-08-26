#define BR_BETA_LOGGING

class BattleRoyalePrepare extends BattleRoyaleState
{
    protected ref array<ref PlayerBase> m_PlayerList;
    protected ref array<string> a_StartingItems;



    protected vector world_center;
    void BattleRoyalePrepare()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyalePrepare::Constructor()");
		#endif
        
        BattleRoyaleGameData m_GameSettings = BattleRoyaleConfig.GetConfig().GetGameData();
        if(m_GameSettings)
        {
            a_StartingItems = m_GameSettings.player_starting_items;
        }
        else
        {
            a_StartingItems = {"TrackSuitJacket_Red","TrackSuitPants_Red","JoggingShoes_Red"}; 
        }
        
        
        m_PlayerList = new array<ref PlayerBase>;

        string path = "CfgWorlds " + GetGame().GetWorldName();
        world_center = GetGame().ConfigGetVector(path + " centerPosition");


    }

	override void Activate()
	{
		super.Activate();

        ServerData m_ServerData = BattleRoyaleAPI.GetAPI().GetCurrentServer();
        while(!m_ServerData || m_ServerData.locked == 0)
        {
            Print("Attempting to lock the server!");
            BattleRoyaleAPI.GetAPI().ServerSetLock(true); //Lock the server
            m_ServerData = BattleRoyaleAPI.GetAPI().GetServer(m_ServerData._id);
        }

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        foreach(PlayerBase player : m_Players)
        {
            m_PlayerList.Insert(player);
        }
        

        GetGame().GameScript.Call(this, "ProcessPlayers", NULL); //Spin up a new thread to process giving players items and teleporting them
	}
	override void Deactivate()
	{
		super.Deactivate();

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(false), true); //fade out screen
	}
    
	override bool IsComplete()
	{
		return super.IsComplete();
	}
    override string GetName()
	{
		return "Prepare State";
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

        //remove all other items
        process_player.RemoveAllItems_Safe();
        foreach(string item : a_StartingItems)
        {
            process_player.GetInventory().CreateInInventory(item);
        }     
    }
    protected void Teleport(PlayerBase process_player)
    {
        vector random_pos = "0 0 0";
        while(true) 
        {
            float edge_pad = 0.1;
            float x = Math.RandomFloatInclusive((world_center[0] * edge_pad), (world_center[0] * 2) - (world_center[0] * edge_pad));
            float z = Math.RandomFloatInclusive((world_center[1] * edge_pad), (world_center[1] * 2) - (world_center[1] * edge_pad));
            float y = GetGame().SurfaceY(x, z);
            
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

            vector start = random_pos + Vector( 0, 5, 0 );
            vector end = random_pos;
            float radius = 2.0; 

            PhxInteractionLayers collisionLayerMask = PhxInteractionLayers.VEHICLE|PhxInteractionLayers.BUILDING|PhxInteractionLayers.DOOR|PhxInteractionLayers.ITEM_LARGE|PhxInteractionLayers.FENCE;
            Object m_HitObject;
            vector m_HitPosition;
            vector m_HitNormal;
            float m_HitFraction;
            bool m_Hit = DayZPhysics.SphereCastBullet( start, end, radius, collisionLayerMask, NULL, m_HitObject, m_HitPosition, m_HitNormal, m_HitFraction );
            Print("Raycast Safe Teleport Position");
            Print(m_Hit);
            Print(m_HitObject);
            Print(m_HitFraction);

            if(m_Hit)
                continue;

            break;
        }

        process_player.SetPosition(random_pos);
    }
    void ProcessPlayers()
    {
        //m_Players is a static list so it won't change during this stage
        for(int i = 0; i < m_Players.Count(); i++)
        {
            PlayerBase process_player = m_Players[i];

            if(process_player)
            {
                GiveStartingItems(process_player);
                
                Teleport(process_player); //TODO: replace this with moving into C130 (for paradrops)
                
            }
        }
    
        Deactivate();
    }
}