modded class ExpansionMapMenu {
    protected ref BattleRoyaleClient m_BattleRoyaleClient;
    override Widget Init()
    {
        BattleRoyaleBase DayZBR = GetBR();
        if(!Class.CastTo(m_BattleRoyaleClient, DayZBR))
        {
            Error("Failed to cast DayZBR to BattleRoyaleClient!");
        } 

        return super.Init();

        //TODO: add custom marker on open?
    }
    
}