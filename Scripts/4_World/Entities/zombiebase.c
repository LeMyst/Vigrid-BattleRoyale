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
                    //TODO: review if this actually does capture all types of zombie kills (what about running them over with a vehicle?)
                    PlayerBase pbKiller;
                    if(Class.CastTo( pbKiller, killer ))
                    {
                        Print("killer aquired");
                        if(pbKiller.GetIdentity())
                        {
                            Print("registering zombie kill");
                            string playerid = pbKiller.GetIdentity().GetPlainId();
                            GetBR().GetMatchData().KillZombie( playerid, this.GetPosition(), GetGame().GetTime() );
                        }
                    }
                    else
                    {
                        Print("zombie killed by unknown entitiy: " + killer.GetDisplayName());
                    }
                }
            }
        }
        super.EEKilled(killer);
    }
}