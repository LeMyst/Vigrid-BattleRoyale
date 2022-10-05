modded class ZombieBase {
    override void EEKilled(Object killer)
	{
        if(killer)
        {
            if(GetGame().IsMultiplayer() && GetGame().IsServer() && GetBR())
            {
                //! server - log zombie kill event!
                if(!GetBR().IsDebug())
                {
                    EntityAI eAI;
                    if(Class.CastTo( eAI, killer ))
                    {
                        Man player = eAI.GetHierarchyRootPlayer(); //get root player of this hierarchy
                        if(player)
                        {
                            PlayerBase pbKiller;
                            if(Class.CastTo( pbKiller, player ))
                            {
                                if(pbKiller.GetIdentity())
                                {
                                    string playerid = pbKiller.GetIdentity().GetPlainId();
                                    GetBR().GetMatchData().KillZombie( playerid, this.GetPosition(), GetGame().GetTime() );
                                }
                            }
                        }
                    }
                }
            }
        }
        super.EEKilled(killer);
    }
}