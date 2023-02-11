class BattleRoyaleLootDataField
{
    protected ref BattleRoyaleLootCategory p_Category;
    protected ref array<ref BattleRoyaleLootEntry> a_Entries;

    protected float total_weight; //this will help us improve efficiency



    void BattleRoyaleLootDataField( ref BattleRoyaleLootCategory category )
    {
        total_weight = 0;
        p_Category = category;
        a_Entries = new array<ref BattleRoyaleLootEntry>;
    }

    void ~BattleRoyaleLootDataField()
    {
        delete a_Entries;
    }

    void AddEntry(ref BattleRoyaleLootEntry entry)
    {
        a_Entries.Insert( entry );
        total_weight += entry.weight;
    }

    string GetName()
    {
        return p_Category.GetName();
    }
    float GetWeight()
    {
        return p_Category.weight;
    }

    ref array<ref BattleRoyaleLootEntry> GetEntries()
    {
        return a_Entries;
    }



    ref BattleRoyaleLootEntry GetRandomEntryByWeight()
    {
        float cur_value = 0;
        float value = Math.RandomFloat(0, total_weight);
        for(int i = 0; i < a_Entries.Count(); i++)
        {
            ref BattleRoyaleLootEntry entry = a_Entries[i];

            cur_value += entry.weight;

            if(cur_value > value)
            {
                return entry;
            }
        }
        return NULL;
    }
}
