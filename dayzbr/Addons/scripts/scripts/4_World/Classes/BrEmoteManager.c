modded class EmoteManager
{
	//ref BattleRoyaleRound m_BattleRoyaleRound;
	static bool Prepare_Players
	
	static void m_Prepare_Players(bool x)
	{
		Prepare_Players = x;
	}
	
	override void OnEmoteEnd()
	{
		if ( m_ItemToHands )
		{
			ShowItemInHands();
		}
		
		//if suicide has been completed, kill player
		if ( m_PlayerDies )
		{
			m_Player.SetHealth(0.0);
			return;
		}
		
		if ( m_BelayedEmote )
		{
			if ( m_PreviousGestureID != m_BelayedEmoteID )
			{
				PickEmote(m_BelayedEmoteID);
				//PlayEmote(m_BelayedEmoteID);
			}
			m_BelayedEmoteID = -1;
			m_BelayedEmote = false;
		}
		else
		{
			if ( m_Player.GetActionManager() ) m_Player.GetActionManager().EnableActions();
			m_Player.m_InventorySoftLocked = false;
			
			if ( m_controllsLocked )
			{
				if(!Prepare_Players)
				{
					m_controllsLocked = false;
					m_Player.GetInputController().OverrideAimChangeX(false,0);
					m_Player.GetInputController().OverrideMovementSpeed(false,0);
				}
			}
		}

		//! back to the default - shoot from camera - if not set already
		if (!m_Player.IsShootingFromCamera()) m_Player.OverrideShootFromCamera(true);
	}
};
