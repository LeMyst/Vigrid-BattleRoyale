#define BR_BETA_LOGGING

class BattleRoyalePrepare extends BattleRoyaleState
{
    protected ref array<ref PlayerBase> m_PlayerList;
    protected bool ready_to_process;
    protected int process_index;

    void BattleRoyalePrepare()
    {
        #ifdef BR_BETA_LOGGING
		BRPrint("BattleRoyalePrepare::Constructor()");
		#endif
        m_PlayerList = new array<ref PlayerBase>;
        ready_to_process = false;
        process_index = 0;
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
                    process_player.RemoveAllItems();
                    //TODO: make this a config setting
                    player.GetInventory().CreateInInventory("TrackSuitJacket_Red");
                    player.GetInventory().CreateInInventory("TrackSuitPants_Red");
                    player.GetInventory().CreateInInventory("JoggingShoes_Red");
                
                    //TODO: teleport players randomly across the map? (move them into the plane to drop)
                }
                process_index++;
            }
        }
    }
}