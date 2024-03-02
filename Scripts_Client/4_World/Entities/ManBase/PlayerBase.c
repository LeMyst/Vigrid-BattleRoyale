modded class PlayerBase
{
    //credit to wardog for the quick fix for client localplayers grabbing
    private static autoptr array<PlayerBase> s_LocalPlayers = new array<PlayerBase>();

#ifndef SERVER
    void PlayerBase()
    {
        if (s_LocalPlayers)
        {
            s_LocalPlayers.Insert(this);
        }
    }

    void ~PlayerBase()
    {
        if (s_LocalPlayers)
        {
            int localIndex = s_LocalPlayers.Find(this);
            if (localIndex >= 0)
            {
                s_LocalPlayers.Remove(localIndex);
            }
        }
    }
#endif

    static void GetLocalPlayers(out array<PlayerBase> players)
    {
#ifndef SERVER
        players = new array<PlayerBase>();
        players.Copy(s_LocalPlayers);
#endif
    }
}
