modded class Grenade_Base
{
    protected string m_ActivatorId = "";
    

    string GetActivator()
    {
        return m_ActivatorId;
    }

    override void EEKilled(Object killer)
	{
        //TODO: if killer is a player, then 
        PlayerBase pbKiller;
        if(Class.CastTo( pbKiller, killer ))
        {
            if(pbKiller.GetIdentity())
            {
                m_ActivatorId = pbKiller.GetIdentity().GetPlainId();
            }
        }
        super.EEKilled(killer);
    }
    protected override void OnUnpin()
	{
        super.OnUnpin();
        if(GetHierarchyRootPlayer())
        {
            PlayerBase pbUser;
            if(Class.CastTo( pbUser, GetHierarchyRootPlayer() ))
            {
                if(pbUser.GetIdentity())
                {
                    m_ActivatorId = pbUser.GetIdentity().GetPlainId();
                }
            }
        }
    }
}