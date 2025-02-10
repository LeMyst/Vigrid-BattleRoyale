class BattleRoyaleBase
{
    void BattleRoyaleBase() {}

    void OnPlayerTick(PlayerBase player, float timeslice) {}
    
    void OnPlayerKilled(PlayerBase killed, Object source) {}

    void Update(float delta) {}

    bool IsDebug()
    {
        return true;
    }
}
