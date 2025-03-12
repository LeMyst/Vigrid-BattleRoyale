#ifdef SERVER
modded class Grenade_Base
{
	protected string m_ActivatorId = "";

	string GetActivator()
	{
		return m_ActivatorId;
	}

	// Someone destroyed the grenade and made it explode
	override void EEKilled(Object killer)
	{
		PlayerBase pbKiller;
		if( Class.CastTo( pbKiller, killer ) )
		{
			if( pbKiller.GetIdentity() )
			{
				m_ActivatorId = pbKiller.GetIdentity().GetPlainId();
			}
		}
		super.EEKilled( killer );
	}

	// Someone unpin the grenade
	override void OnUnpin()
	{
		super.OnUnpin();
		if( GetHierarchyRootPlayer() )
		{
			PlayerBase pbUser;
			if( Class.CastTo( pbUser, GetHierarchyRootPlayer() ) )
			{
				if( pbUser.GetIdentity() )
				{
					m_ActivatorId = pbUser.GetIdentity().GetPlainId();
				}
			}
		}
	}
}
#endif
