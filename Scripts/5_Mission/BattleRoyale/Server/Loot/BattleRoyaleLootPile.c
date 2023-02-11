class BattleRoyaleLootPile
{
    protected ref BattleRoyaleLootableBuilding m_Parent;
    protected vector v_ModelPosition;
    protected vector v_WorldPosition;

    protected bool b_IsSpawned;
    protected bool b_Active;

    protected ref BattleRoyaleLootEntry m_Entry;

    protected ref array<string> class_names;
    protected ref array<string> class_names_attachments;
    protected ref array<ItemBase> spawned_items;

    void BattleRoyaleLootPile(vector model_pos, ref BattleRoyaleLootableBuilding parent )
    {
        spawned_items = new array<ItemBase>();
        class_names = new array<string>();
        class_names_attachments = new array<string>();
        v_ModelPosition = model_pos;
        m_Parent = parent;
        b_IsSpawned = false;

        v_WorldPosition = "0 0 0";

        float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_pile;
        b_Active = (Math.RandomFloat(0, 1) < odds);
    }

    /*
        //TODO: LOOT RAYCASTING #1

        //--- raycast position - attempt to reflect dayz's spawning system.
        the idea here is we'll get "model_pos", "radius", and "height" as an input,
        if b_Active (above) then run this raycast
        if the raycast is successful, GetWorldPos() should return the raycast position result

        vector start = Vector(0, 0 ,0); //this is the ModelToWorld of the Point "pos" field
        vector end = start + Vector(0, height, 0); //height is the "height" field
        float radius = 0; //radius is the "radius" field

        ref RaycastRVParams params = new RaycastRVParams( start, end, null, radius);
        params.type = ObjIntersectIFire;

        ref array< ref RaycastRVResult> results = new array<ref RaycastRVResult>();
        if(RaycastRVProxy( params, results ))
        {
            if(results.Count() > 0)
            {
                vector pos = results[0].pos; //use the first collision we use nearestcontact in the params)
                return pos;
            }
        }
    */

    vector GetWorldPos()
    {
        return m_Parent.GetObject().ModelToWorld( v_ModelPosition );
    }

    void Init()
    {
        v_WorldPosition = GetWorldPos(); //calculate our world position
        Print(v_WorldPosition);
        //select a random loot entry! (this is all done in the background)
        m_Entry = BattleRoyaleLootData.GetData().GetRandomFieldByWeight().GetRandomEntryByWeight();

        string class_name =  m_Entry.GetRandomStyle();
        class_names.Insert( class_name ); //insert base item name
        class_names.InsertAll( m_Entry.spawn_with ); //insert all items that need to spawn with

        int i;
        int j;
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
            ref map<string, ref array<ref BattleRoyaleLootEntry>> all_slot_entries = BattleRoyaleLootData.GetData().GetAllAttachmentEntries( class_name );

            if(all_slot_entries.Count() > 0)
            {
                //odds for spawning an attachment slot
                float odds = BattleRoyaleLootSettings.Cast( BattleRoyaleConfig.GetConfig().GetConfig("LootData") ).chance_to_spawn_attachment;

                for(i = 0; i < all_slot_entries.Count(); i++)
                {
                    float roll = Math.RandomFloat(0, 1);
                    if(roll < odds)
                    {
                        //this slot will be populated!
                        ref array<ref BattleRoyaleLootEntry> attachment_entries = all_slot_entries.GetElement(i);
                        int count = attachment_entries.Count();
                        if(count > 0)
                        {
                            float max = 0;
                            for(j = 0; j < count; j++)
                            {
                                max += attachment_entries[j].weight;
                            }
                            float res = Math.RandomFloat(0, max); //roll between 0 and max
                            float val = 0;
                            ref BattleRoyaleLootEntry entry;
                            for(j = 0;j < count; j++)
                            {
                                entry = attachment_entries[j];

                                val += entry.weight;
                                if(val > res) //if we iterated above our result, than this item is the one we want
                                {
                                    break; //this entry is correct so exit the for loop
                                }
                            }

                            string attachment_classname = entry.GetRandomStyle();
                            //class_names.Insert( attachment_classname );
                            class_names_attachments.Insert( attachment_classname );

                            // Figure out if this needs batteryd
                            string configPath = "CfgVehicles " + attachment_classname + " attachments";
                            ref array<string> attach_subattach_list = new array<string>();
                            GetGame().ConfigGetTextArray(configPath, attach_subattach_list);
                            if(attach_subattach_list.Find("BatteryD") != -1)
                            {
                                //class_names.Insert( "Battery9V" ); //spawn with battery
                                class_names_attachments.Insert( "Battery9V" );
                            }
                        }
                        else
                        {
                            Error("Slot Attachments Contains no Entries!");
                        }
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
        Print("CreateItem");
        Print(v_WorldPosition);
        return ItemBase.Cast( GetGame().CreateObjectEx( class_name, v_WorldPosition, ECE_PLACE_ON_SURFACE ) );
        //return ItemBase.Cast(GetGame().CreateObject(class_name, v_WorldPosition));
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
                if(item.IsInherited(Weapon_Base))
                {
                    Weapon_Base weapon_base = Weapon_Base.Cast( item );
                    weapon_base.SpawnAmmo("", WeaponWithAmmoFlags.CHAMBER | WeaponWithAmmoFlags.QUANTITY_RNG );
                }

                if(item.GetInventory().GetAttachmentSlotsCount() > 0)
                {
                    for(int j = 0; j < class_names_attachments.Count(); j++)
                    {
                        Print("==> Attachment " + class_names_attachments[j]);
                        EntityAI item_attachment = item.GetInventory().CreateInInventory( class_names_attachments[j] );
                        if(!item_attachment)
                        {
                            Error("Can't add attachment `" + class_names_attachments[j] + "`! Spawn it independently!");
                            ItemBase new_attachment = CreateItem( class_names_attachments[j] );
                            if(new_attachment)
                            {
                                // We add the un-added attachment to the loot pile
                                spawned_items.Insert( new_attachment );
                            }
                        } else {
                            Print("Attachment added");
                        }
                    }
                }
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

        //TODO: instead of disabling activity when moved,
        // Remove the GetType() name from `class_names` if the item was moved.
        // This way unmoved items in the pile can still despawn & save perf
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
