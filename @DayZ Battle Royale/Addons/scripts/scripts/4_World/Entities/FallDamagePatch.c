modded class DayZPlayerImplementFallDamage
{
	//! handle fall damage
	void HandleFallDamage(float pHeight)
	{
		if (GetGame().IsServer())
		{
			//! no dmg height
			if (pHeight <= FD_DMG_FROM_HEIGHT)
				return;

			PlayerBase pb = PlayerBase.Cast(m_Player);
			if(pb)
			{
				if(pb.BR_BASE)
				{
					if(!pb.BR_BASE.allowFallDamage(pb))
					{
						return;
					}
				}
			}
			
			vector posMS = m_Player.WorldToModel(m_Player.GetPosition());

			//! global dmg multiplied byt damage coef
			m_Player.ProcessDirectDamage( DT_FALL, m_Player, "", FD_AMMO, posMS, DamageCoef(pHeight) );

			//! updates injury state of player immediately
			
			if (pb) pb.ForceUpdateInjuredState();
		}
	}
	
}