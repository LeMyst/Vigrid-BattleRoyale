#define BR_BETA_LOGGING

class BattleRoyalePrepare extends BattleRoyaleState
{
    protected ref array<ref PlayerBase> m_PlayerList;
    protected bool ready_to_process;
    protected int process_index;
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
            a_StartingItems = {"TrackSuitJacket_Red","TrackSuitPants_Red","JoggingShoes_Red"}; //TODO: add GPS and MAP to these items
        }
        
        
        m_PlayerList = new array<ref PlayerBase>;
        ready_to_process = false;
        process_index = 0;

        string path = "CfgWorlds " + GetGame().GetWorldName();
        world_center = GetGame().ConfigGetVector(path + " centerPosition");


    }

	override void Activate()
	{
		super.Activate();

        //TODO: spawn & setup drop plane
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetInput", new Param1<bool>(true), true); //disable user input
        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(true), true); //fade out screen

        //we process on a static list so when players possibly disconnect during this phase we don't error out or skip any clients
        foreach(PlayerBase player : m_Players)
        {
            m_PlayerList.Insert(player);
        }
        ready_to_process = true;
	}
	override void Deactivate()
	{
		super.Deactivate();

        GetRPCManager().SendRPC( RPC_DAYZBR_NAMESPACE, "SetFade", new Param1<bool>(false), true); //fade out screen
	}
    
	override bool IsComplete()
	{
		return (ready_to_process && (process_index >= m_PlayerList.Count())) || super.IsComplete();
	}
    override string GetName()
	{
		return "Prepare State";
	}
    override void Update(float timeslice)
    {
        super.Update(timeslice);

        if(ready_to_process)
        {
            if(process_index < m_PlayerList.Count())
            {
                PlayerBase process_player = m_PlayerList.Get(process_index);
                #ifdef BR_BETA_LOGGING
                BRPrint("BattleRoyalePrepare::Update() => Processing player # " + process_index.ToString());
                #endif
                
                if(process_player)
                {
                    process_player.RemoveAllItems_Safe();
                    foreach(string item : a_StartingItems)
                    {
                        process_player.GetInventory().CreateInInventory(item);
                    }
                    
                    //TODO: teleport players randomly across the map? (move them into the plane to drop)
                   
                    vector random_pos = "0 0 0";
                    float edge_pad = 0.1;
                    float x = Math.RandomFloatInclusive((world_center[0] * edge_pad), (world_center[0] * 2) - (world_center[0] * edge_pad));
                    float z = Math.RandomFloatInclusive((world_center[1] * edge_pad), (world_center[1] * 2) - (world_center[1] * edge_pad));
                    float y = GetGame().SurfaceY(x, z);
                    //TODO: ensure not in water
                    random_pos[0] = x;
                    random_pos[1] = y;
                    random_pos[2] = z;
                    process_player.SetPosition(random_pos);
                }
                process_index++;
            }
        }
    }
}