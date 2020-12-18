class BattleRoyaleBans
{
    static ref BattleRoyaleBans m_Instance;
    static BattleRoyaleBans GetBans()
    {
        if(!m_Instance)
        {
            //init ban system
            Print("INITIALIZING BATTLEROYALE BANS API");
            m_Instance = new BattleRoyaleBans;
        }
        return m_Instance;
    }

    void WriteBanlist()
    {
        string filepath = "$CurrentDir:ban.txt";

        /*
            NOTE: in order to write to this file path, we'll need to implement a workaround.
                1. Either symlink $profile:ban.txt to $CurrentDir:ban.txt and use profile as our file path
                2. Apply a patch to enable writing to $CurrentDir: (http://lystic.net/2020/10/24/enabling-file-write-to-dayz-root-directory/)
        */

        int hBans = OpenFile( filepath, FileMode.WRITE ); 
        if(hBans == 0)
        {
            Error("WriteBanlist() => Failed to open handle to ban.txt");
            return;
        }

        string banlist = BattleRoyaleAPI.GetAPI().GetBanlist();
        if(banlist.Contains("{\"error\""))
        {
            Error("WriteBanlist() => API returned error! " + banlist);   
            CloseFile(hBans);  
            return;  
        }
        
        FPrint(hBans, banlist);
        CloseFile(hBans);

        Print("BANLIST UPDATED!");
    }

}