
class BattleRoyaleLootPile
{
    protected ref BattleRoyaleLootableBuilding m_Parent;
    protected vector v_ModelPosition;
    protected vector v_WorldPosition;

    protected bool b_IsSpawned;
    protected bool b_Active;

    protected ref BattleRoyaleLootEntry m_Entry;

    protected ref array<string> class_names;
    protected ref array<ItemBase> spawned_items;


    void BattleRoyaleLootPile(vector model_pos, ref BattleRoyaleLootableBuilding parent )
    {
        spawned_items = new array<ItemBase>();
        class_names = new array<string>();
        v_ModelPosition = model_pos;
        m_Parent = parent;
        b_IsSpawned = false;

        v_WorldPosition = "0 0 0";


        float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_pile;
        b_Active =(Math.RandomFloat(0, 1) < odds); 
    }
    
    vector GetWorldPos()
    {
        return m_Parent.GetObject().ModelToWorld( v_ModelPosition );
    }

    void Init()
    {
        v_WorldPosition = GetWorldPos(); //calculate our world position
        //select a random loot entry! (this is all done in the background)
        m_Entry = BattleRoyaleLootData.GetData().GetRandomFieldByWeight().GetRandomEntryByWeight();

        string class_name =  m_Entry.GetRandomStyle();
        class_names.Insert( class_name ); //insert base item name
        class_names.InsertAll( m_Entry.spawn_with ); //insert all items that need to spawn with

        int i;
        int index;

        if(m_Entry.SpawnWithMagazines())
        {
            //cached lookup of all possible magazines
            ref array<string> all_mags = BattleRoyaleLootData.GetData().GetAllMagazines( class_name );

            if(all_mags.Count() > 0)
            {
                int num_mags = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).num_mags_to_spawn_with;

                for(i = 0; i < num_mags; i++)
                {
                    index = Math.RandomInt(0, all_mags.Count());
                    class_names.Insert( all_mags[index] );
                }
            }
            else
            {
                Error("Trying to spawn `" + class_name + "` with mags, but none are found!");
            }
            
        }
        if(m_Entry.SpawnWithAmmo())
        {

            ref array<string> all_ammo = BattleRoyaleLootData.GetData().GetAllAmmo( class_name );
            
            if(all_ammo.Count() > 0)
            {
                int num_ammo = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).num_ammoboxs_to_spawn_with;

                for(i = 0; i < num_ammo; i++)
                {
                    index = Math.RandomInt(0, all_ammo.Count());
                    class_names.Insert( all_ammo[index] );
                }
            }
            else
            {
                Error("Trying to spawn `" + class_name + "` with ammo, but none are found!");
            }

        }
        if(m_Entry.SpawnWithAttachments())
        {
            ref array<string> all_attachments = BattleRoyaleLootData.GetData().GetAllAttachments( class_name );

            if(all_attachments.Count() > 0)
            {
                float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_attachment;

                for(i = 0; i < all_attachments.Count(); i++)
                {
                    float roll = Math.RandomFloat(0, 1);
                    if(roll < odds)
                    {
                        class_names.Insert( all_attachments[i] );
                    }
                }
            }
            else
            {
                Error("Trying to spawn `" + class_name + "` with attachments, but none are found!");
            }
        }
    }

    protected ItemBase CreateItem(string class_name)
    {
        return ItemBase.Cast(GetGame().CreateObject(class_name, v_WorldPosition));
    }

    void Spawn()
    {
        if(!b_Active)
            return;

        if(b_IsSpawned)
        {
            Error("Trying to spawn loot that is already spawned!");
            return;
        }
        if(spawned_items.Count() != 0)
        {
            Error("Trying to spawn loot but some items are already spawned!");
            return;
        }
        
        if(!m_Entry)
            Init();
        
        

        Print("Spawning Items");
        for(int i = 0; i < class_names.Count(); i++)
        {
            Print("==> " + class_names[i]);
            ItemBase item = CreateItem( class_names[i] );
            if(item)
            {
                spawned_items.Insert(item);
            }
            else
            {
                Error("Failed to create item `" + class_names[i] + "`!");
            }
        }

        b_IsSpawned = true;
    }
    void Despawn()
    {
        if(!b_Active)
            return;

        if(!b_IsSpawned)
        {
            Error("Trying to despawn loot that is not spawned!");
            return;
        }

        Print("Despawning Loot Pile");

        int i;
        
        bool was_moved = false;
        for(i = 0; i < spawned_items.Count(); i++)
        {
            if(!(spawned_items[i]))
            {
                was_moved = true;
                break;
            }
            vector item_pos = spawned_items[i].GetPosition();
            float dist = vector.Distance( item_pos, v_WorldPosition);
            if(dist > 1)
            {
                was_moved = true;
                break;
            }
        }
        if(was_moved)
        {
            b_Active = false;
            return;
        }
        for(i = 0; i < spawned_items.Count(); i++)
        {
            GetGame().ObjectDelete( spawned_items[i] );
        }
        spawned_items.Clear();
        b_IsSpawned = false;
    }

}