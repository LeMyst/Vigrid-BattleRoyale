class BattleRoyaleSpectators
{
    protected ref array<PlayerBase> m_Players;
    protected ref ScriptCallQueue m_CallQueue;

    void BattleRoyaleSpectators()
    {
        m_Players = new array<PlayerBase>();
        m_CallQueue = new ScriptCallQueue;
    }

    bool CanSpectate(PlayerBase player)
    {
        //TODO: create a config of allowed players for spectating (or a config to enable global spectate)
        return false; 
    }
    bool ContainsPlayer(PlayerBase player)
    {
        return (m_Players.Find(player) != -1);
    }
    void AddPlayer(PlayerBase player)
    {
        m_Players.Insert(player);
        //spin up a thread to handle spectator camera initialization
        GetGame().GameScript.Call(this, "InitSpectatorCamera", player); 
    }
    void Update(float delta)
    {
        m_CallQueue.Tick( delta );
    }
    void OnPlayerTick(PlayerBase player, float delta)
    {
        // just in case we need it
    }

    protected void InitSpectatorCamera(PlayerBase player)
    {
        //we need to initialize some system on this client that lets them freecam around while updating their network bubble to the camera
    }
}