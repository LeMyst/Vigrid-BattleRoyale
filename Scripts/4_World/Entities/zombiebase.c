modded class ZombieBase {
    override void EEKilled(Object killer)
	{
        if(GetGame().IsServer() && GetBR())
        {
            //! server - log zombie kill event!
            BattleRoyaleDebug m_Debug;
            BattleRoyaleState m_CurrentState = BattleRoyaleServer.Cast( GetBR() ).GetCurrentState();
            if(!Class.CastTo(m_Debug, m_CurrentState))
            {
                //TODO: review if this actually does capture all types of zombie kills (what about running them over with a vehicle?)
                PlayerBase pbKiller;
                if(Class.CastTo( pbKiller, killer ))
                {
                    if(pbKiller.GetIdentity())
                    {
                        string playerid = pbKiller.GetIdentity().GetPlainId();
                        BattleRoyaleServer.Cast(  GetBR() ).GetMatchData().KillZombie( playerid, this.GetPosition(), GetGame().GetTime() );
                    }
                }
            }
        }
        super.EEKilled(killer);
    }
}