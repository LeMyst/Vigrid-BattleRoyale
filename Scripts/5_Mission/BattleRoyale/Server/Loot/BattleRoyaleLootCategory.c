class BattleRoyaleLootCategory {
    [NonSerialized()]
    string m_FileName;

    float weight;

    void Load(string filename)
    {
        JsonFileLoader<BattleRoyaleLootCategory>.JsonLoadFile(BATTLEROYALE_LOOT_CATEGORIES_FOLDER + filename , this);
        this.m_Filename = filename;
    }

    string GetName()
    {
        return m_FileName.Substring(0, m_FileName.IndexOf(".")); //ignore the file extension
    }
}