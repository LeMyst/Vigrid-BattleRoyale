modded class ZombieBase {
    override void EEKilled(Object killer)
	{
        if(killer)
        {
            Print("zombie killed!");

            if(GetGame().IsMultiplayer() && GetGame().IsServer() && GetBR())
            {
                //! server - log zombie kill event!
                if(!GetBR().IsDebug())
                {
                    Print("attempting to get killer!");

                    EntityAI eAI;
                    if(Class.CastTo( eAI, killer ))
                    {
                        Man player = eAI.GetHierarchyRootPlayer(); //get root player of this hierarchy
                        if(player)
                        {
                            PlayerBase pbKiller;
                            if(Class.CastTo( pbKiller, player ))
                            {
                                Print("killer aquired");
                                if(pbKiller.GetIdentity())
                                {
                                    Print("registering zombie kill");
                                    string playerid = pbKiller.GetIdentity().GetPlainId();
                                    GetBR().GetMatchData().KillZombie( playerid, this.GetPosition(), GetGame().GetTime() );
                                }
                            }
                        }
                    }
                    else
                    {
                        Print("zombie killed by unknown object: " + killer.GetDisplayName());
                    }
                }
            }
        }
        super.EEKilled(killer);
    }
}