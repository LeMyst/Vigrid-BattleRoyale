class BattleRoyaleLootData
{
    static ref BattleRoyaleLootData GetData()
    {
        if(!m_Singleton)
        {
            m_Singleton = new BattleRoyaleLootData;
            m_Singleton.Load();
        }
        return m_Singleton;
    }
    static ref BattleRoyaleLootData m_Singleton;

    protected ref map<string, ref BattleRoyaleLootDataField> m_DataFields;

    float total_weight = 0; //used for efficiency

    //--- cached item maps
    protected ref map<string, ref array<string>> m_Magazines;
    protected ref map<string, ref array<string>> m_Ammo;
    protected ref map<string, ref map<string, ref array<ref BattleRoyaleLootEntry>>> m_AttachmentsEntries; //weapon => [ slot => [ entry, entry, ...], ...]

    void BattleRoyaleLootData()
    {
        m_DataFields = new map<string, ref BattleRoyaleLootDataField>();
        m_Magazines = new map<string, ref array<string>>();
        m_Ammo = new map<string, ref array<string>>();
        m_AttachmentsEntries = new map<string, ref map<string, ref array<ref BattleRoyaleLootEntry>>>();
    }

    protected void AddCategory( ref BattleRoyaleLootCategory category )
    {
        ref BattleRoyaleLootDataField field = new BattleRoyaleLootDataField( category );
        string name = field.GetName();
        name.ToLower();
        m_DataFields.Insert(name, field);
        total_weight += category.weight;
    }

    protected void AddEntry( ref BattleRoyaleLootEntry entry, bool is_recurs = false )
    {
        string category_name = entry.category;
        category_name.ToLower();
        if(category_name == "")
        {
            Error("Cannot add loot entry with NULL category");
            Print(entry);
            return;
        }
        if(m_DataFields.Contains(category_name))
        {
            m_DataFields.Get( category_name ).AddEntry( entry );
        }
        else
        {
            if(is_recurs)
            {
                Error("Failed to add the correct category!");
                return;
            }
            Error("No Category Found `" + entry.category + "`");
            ref BattleRoyaleLootCategory category = new BattleRoyaleLootCategory();
            category.weight = 1.0; //default the new categories weight to 1.0
            AddCategory( category );
            AddEntry( entry , true ); //mark as true so we don't recursively do this
        }
    }

    private int LastIndexOf(string str, string substr)
    {
        int index = str.IndexOf(str);
        if(index == -1)
            return -1;
        
        int val = index;
        while(val != -1)
        {
            val = str.IndexOfFrom(val+1, substr);
            if(val != -1)
            {
                index = val;
            }
        }
        return index;
    }

    void Load()
    {
        Print("[Loot] Loading Loot Data...");
        //load categories & instantiate loot data fields
        array< string > files = FindFilesInLocation( BATTLEROYALE_LOOT_CATEGORIES_FOLDER );
        int i;
        string fileName;
        int pos;
        string ext;
        int start_index;
        int length;
        for ( i = 0; i < files.Count(); i++ )
		{
			pos = LastIndexOf( files[i], "." );
            if ( pos > -1 )
			{
                start_index = pos + 1;
                length = files[i].Length() - start_index;

                Print(files[i]);
                Print(start_index);
                Print(length);
                ext = files[i].Substring(start_index, length);
                Print(ext);
                if(ext == "json")
                {
                    fileName = files[i];
                    //--- construct category object from file
                    ref BattleRoyaleLootCategory category = new BattleRoyaleLootCategory();
                    Print("[Loot] Loading Category " + fileName);
                    category.Load(fileName);
                    
                    AddCategory(category); //insert this category to our fields
                    
                }
                else
                {
                    Print("[Loot] Found category file with invalid extension `" + ext + "`");
                }
            }
        }
        //load entries & populate loot data fields

        files = FindFilesInLocation( BATTLEROYALE_LOOT_ENTRIES_FOLDER );
        for ( i = 0; i < files.Count(); i++ )
		{
            
			pos = LastIndexOf( files[i], "." );
            if ( pos > -1 )
			{
                start_index = pos + 1;
                length = files[i].Length() - start_index;

                ext = files[i].Substring(start_index, length);
                if(ext == "json")
                {
                    fileName = files[i];
                    //--- construct entry from file
                    ref BattleRoyaleLootEntry entry = new BattleRoyaleLootEntry();
                    Print("[Loot] Loading Entry " + fileName);
                    entry.Load(fileName);
                    
                    AddEntry( entry );
                }
                else
                {
                    Print("[Loot] Found entry file with invalid extension `" + ext + "`");
                }
                
            }
        }
    }

    ref BattleRoyaleLootDataField GetRandomFieldByWeight()
    {
        float cur_value = 0;
        float value = Math.RandomFloat(0, total_weight);
        for(int i = 0; i < m_DataFields.Count(); i++)
        {
            ref BattleRoyaleLootDataField field = m_DataFields.GetElement(i);
            
            cur_value += field.GetWeight();

            if(cur_value > value)
            {
                return field;
            }
        }
        return NULL;
    }

    private bool ContainsCaseInsensitive(string find, notnull array<string> search)
    {
        find.ToLower();
        for(int i = 0; i < search.Count(); i++)
        {
            string item = search[i];
            item.ToLower();
            if(find == item)
            {
                return true;
            }
        }
        return false;
    }

    private string GetCollide(notnull array<string> search, notnull array<string> find)
    {
        for(int i = 0; i < find.Count(); i++)
        {
            if(ContainsCaseInsensitive(find[i], search))
                return find[i];
        }
        return "";
    }

    ref array<string> GetAllMagazines(string item_class)
    {
        //lowercase item name
        string normalized_name = item_class;
        normalized_name.ToLower();

        if(m_Magazines.Contains(normalized_name))
        {
            return m_Magazines.Get(normalized_name);
        }

        ref array<string> result = new array<string>();
        if(!m_DataFields.Contains( BATTLEROYALE_LOOT_MAGAZINES_CATEGORY ))
        {
            Error("No magazines category!");
            return result;
        }

        //get all possible magazines for this item
        string configPath = "CfgWeapons " + normalized_name + " magazines";
        ref array<string> magazines = new array<string>(); 
        GetGame().ConfigGetTextArray(configPath,magazines);

        ref array<ref BattleRoyaleLootEntry> entries = m_DataFields.Get( BATTLEROYALE_LOOT_MAGAZINES_CATEGORY ).GetEntries();

        for(int i = 0; i < entries.Count();i++)
        {
            for(int j = 0; j < entries[i].styles.Count(); j++)
            {
                string mag_name = entries[i].styles[j];
                if(ContainsCaseInsensitive(mag_name, magazines))
                {
                    result.Insert(mag_name);
                }
            }
        }

        m_Magazines.Insert( normalized_name, result );

        return result;

    }

    ref array<string> GetAllAmmo(string item_class)
    {
        //lowercase item name
        string normalized_name = item_class;
        normalized_name.ToLower();

        if(m_Ammo.Contains(normalized_name))
        {
            return m_Ammo.Get(normalized_name);
        }

        ref array<string> result = new array<string>();
        if(!m_DataFields.Contains( BATTLEROYALE_LOOT_AMMO_CATEGORY ))
        {
            Error("No ammo category!");
            return result;
        }

        //get all possible ammo for this item
        string configPath = "CfgWeapons " + normalized_name + " chamberableFrom";
        ref array<string> ammo = new array<string>(); 
        GetGame().ConfigGetTextArray(configPath,ammo);

        ref array<ref BattleRoyaleLootEntry> entries = m_DataFields.Get( BATTLEROYALE_LOOT_AMMO_CATEGORY ).GetEntries();

        for(int i = 0; i < entries.Count();i++)
        {
            for(int j = 0; j < entries[i].styles.Count(); j++)
            {
                string ammo_name = entries[i].styles[j];

                //i don't know if this is a round, or a box of ammo so let's try both
                if(ContainsCaseInsensitive(ammo_name, ammo))
                {
                    result.Insert(ammo_name);
                }
                else
                {
                    string resources_path = "CfgVehicles " + ammo_name + " Resources";
                    if(GetGame().ConfigIsExisting(resources_path))
                    {
                        int resource_count = GetGame().ConfigGetChildrenCount(resources_path);
                        for(int k = 0; k < resource_count; k++)
                        {
                            string out_name;
                            if(GetGame().ConfigGetChildName(resources_path, k, out_name))
                            {
                                if(ContainsCaseInsensitive(out_name, ammo))
                                {
                                    result.Insert(ammo_name);
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
        return result;
    }

    ref map<string, ref array<ref BattleRoyaleLootEntry>> GetAllAttachmentEntries(string item_class)
    {
        string normalized_name = item_class;
        normalized_name.ToLower();

        if(m_AttachmentsEntries.Contains(normalized_name))
        {
            return m_AttachmentsEntries.Get(normalized_name);
        }

        //"slot" => { entry, entry, entry }
        ref map<string, ref array<ref BattleRoyaleLootEntry>> result = new map<string, ref array<ref BattleRoyaleLootEntry>>();

        if(!m_DataFields.Contains( BATTLEROYALE_LOOT_ATTACHMENTS_CATEGORY ))
        {
            Error("No attachments category!");
            return result;
        }

        ref array<string> weapon_slots = new array<string>(); 
        string configPath = "CfgWeapons " + normalized_name + " attachments";
        GetGame().ConfigGetTextArray(configPath,weapon_slots);

        ref array<ref BattleRoyaleLootEntry> entries = m_DataFields.Get( BATTLEROYALE_LOOT_ATTACHMENTS_CATEGORY ).GetEntries();

        for(int i = 0; i < entries.Count(); i++)
        {
            for(int j = 0; j < entries[i].styles.Count(); j++)
            {
                ref BattleRoyaleLootEntry entry = entries[i];

                string attachment_name = entry.styles[j];

                configPath = "CfgVehicles " + attachment_name + " inventorySlot";
                ref array<string> attachment_slots = new array<string>(); 
                GetGame().ConfigGetTextArray(configPath,attachment_slots);

                string matching_slot = GetCollide(weapon_slots, attachment_slots);
                matching_slot.ToLower();
                if(matching_slot != "")
                {
                    if(!result.Contains(matching_slot))
                    {
                        result.Insert(matching_slot, new array<ref BattleRoyaleLootEntry>()); //insert this field
                    }
                    if(j != 0)
                    {
                        Error("Found matching attachment style (" + attachment_name + ") that isn't first style (mismatching styles in attachment?)");
                    }
                    result.Get(matching_slot).Insert( entry );//insert our attachment into the slot field
                    break; //force iterate to the next entry
                }
            }
        }

        m_AttachmentsEntries.Insert(normalized_name, result);

        return result;
    }
}

