class BattleRoyaleLootEntry : Managed {
    [NonSerialized()]
    string m_FileName;

    string category;
    float weight;
    ref array<string> styles;
    ref array<string> spawn_with;
    int needs_magazines;
    int needs_ammo;
    int needs_attachments;

    void Load(string filename)
    {
        JsonFileLoader<BattleRoyaleLootEntry>.JsonLoadFile(BATTLEROYALE_LOOT_ENTRIES_FOLDER + filename , this);
        this.m_FileName = filename;
    }

    //--- functionality
    string GetRandomStyle()
    {
        int count = styles.Count();
        if(count == 1)
            return styles[0];
        if(count == 0)
            return BATTLEROYALE_LOOT_BROKEN_STYLE_ITEM_CLASSNAME; //anything broken will be Zucchini

        int ind = Math.RandomInt(0, count);
        return styles[ind];
    }

    bool SpawnWithMagazines()
    {
        return (needs_magazines == 1);
    }

    bool SpawnWithAmmo()
    {
        return (needs_ammo == 1);
    }

    bool SpawnWithAttachments()
    {
        return (needs_attachments == 1);
    }
}
