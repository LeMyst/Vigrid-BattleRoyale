#ifdef SERVER
modded class LandMineTrap
{
	protected string m_ActivatorId = "";

	string GetActivatorId()
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

	override void OnPlacementComplete(Man player, vector position = "0 0 0", vector orientation = "0 0 0")
	{
		super.OnPlacementComplete(player, position, orientation);

		if( player && player.GetIdentity() )
		{
			m_ActivatorId = player.GetIdentity().GetPlainId();
		}
	}
}
#endif
