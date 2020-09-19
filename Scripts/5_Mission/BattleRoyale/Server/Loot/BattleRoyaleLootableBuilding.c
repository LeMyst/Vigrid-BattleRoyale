class BattleRoyaleLootableBuilding 
{
    protected Object m_Object;
    protected ref array<PlayerBase> a_NearPlayers;
    protected ref array<ref BattleRoyaleLootPile> a_LootPiles;
    protected bool b_Active;
    protected bool b_Initialized;

    

    void BattleRoyaleLootableBuilding( Object obj )
    {
        m_Object = obj;
        a_NearPlayers = new array<PlayerBase>();
        a_LootPiles = new array<ref BattleRoyaleLootPile>();
        b_Initialized = false;

        float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_building;
        b_Active = (Math.RandomFloat(0, 1) < odds);
    } 
    void ~BattleRoyaleLootableBuilding()
    {
        delete a_NearPlayers;
        delete a_LootPiles;
    }

    Object GetObject()
    {
        return m_Object;
    }

    protected void InitLoot()
    {
        ref array<vector> model_positions = LootReader.GetReader().GetAllLootPositions(m_Object.GetType());
        for(int i = 0; i < model_positions.Count(); i++)
        {
            a_LootPiles.Insert( new BattleRoyaleLootPile( model_positions[i], this ) );
        }
        b_Initialized = true;
    }

    void AddNearPlayer(PlayerBase player)
    {
        if(a_NearPlayers.Find(player) == -1)
        {
            Print("Adding Near Player To Building");
            a_NearPlayers.Insert(player);
            if(a_NearPlayers.Count() == 1)
            {
                SpawnLoot();
            }
        }
    }
    void RemoveNearPlayer(PlayerBase player)
    {
        if(a_NearPlayers.Find(player) != -1)
        {
            Print("Removing Near Player From Building");
            a_NearPlayers.RemoveItem(player);
            if(a_NearPlayers.Count() == 0)
            {
                DespawnLoot();
            }
        }   
    }
    protected void SpawnLoot()
    {
        if(!b_Active)
            return;

        if(!b_Initialized)
            InitLoot();
        
        for(int i = 0; i < a_LootPiles.Count(); i++)
        {
            ref BattleRoyaleLootPile loot_pile = a_LootPiles[i];

            loot_pile.Spawn();
        }
    }
    protected void DespawnLoot()
    {
        if(!b_Active)
            return;

        for(int i = 0; i < a_LootPiles.Count(); i++)
        {
            ref BattleRoyaleLootPile loot_pile = a_LootPiles[i];

            loot_pile.Despawn();
        }
    }
}