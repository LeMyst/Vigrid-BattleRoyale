modded class ZombieBase {
    override void EEKilled(Object killer)
	{
        if(GetGame().IsServer() && GetBR())
        {
            //! server - log zombie kill event!
            if(!GetBR().IsDebug())
            {
                //TODO: review if this actually does capture all types of zombie kills (what about running them over with a vehicle?)
                PlayerBase pbKiller;
                if(Class.CastTo( pbKiller, killer ))
                {
                    if(pbKiller.GetIdentity())
                    {
                        string playerid = pbKiller.GetIdentity().GetPlainId();
                        GetBR().GetMatchData().KillZombie( playerid, this.GetPosition(), GetGame().GetTime() );
                    }
                }
            }
        }
        super.EEKilled(killer);
    }
}