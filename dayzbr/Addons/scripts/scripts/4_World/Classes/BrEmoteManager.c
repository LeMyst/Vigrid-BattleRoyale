/*
modded class EmoteManager
{
	//ref BattleRoyaleRound m_BattleRoyaleRound;
	static bool Prepare_Players
	
	static void m_Prepare_Players(bool x)
	{
		Prepare_Players = x;
	}

	override void SetEmoteLockState(bool state)
	{
		//Print("lock state " + state);
			
		if (!m_HandInventoryLocation)
		{
			m_HandInventoryLocation = new InventoryLocation;
			m_HandInventoryLocation.SetHands(m_Player,null);
		}
		
		if (!state)
		{
			if (m_Player.GetInventory().HasInventoryReservation(null, m_HandInventoryLocation))
			{
				//Print("Clearing hand reservation... ");
				m_Player.GetInventory().ClearInventoryReservation( null, m_HandInventoryLocation);
			}
			
			if ( m_Player.GetActionManager() )
				m_Player.GetActionManager().EnableActions();
			
			m_Player.SetInventorySoftLock(false);
						
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
		else
		{
			if (!m_Player.GetInventory().HasInventoryReservation(null, m_HandInventoryLocation))
			{
				m_Player.GetInventory().AddInventoryReservation( null, m_HandInventoryLocation, 10000);
			}
				
			if ( m_Player.GetActionManager() )
				m_Player.GetActionManager().DisableActions();
			
			m_Player.SetInventorySoftLock(true);
			
			//Movement lock in fullbody anims
			//TODO check for better solution to prevent turning on back and borking the animation (surrender)
			if (m_Callback && m_Callback.m_IsFullbody && !m_controllsLocked && m_CurrentGestureID == ID_EMOTE_SURRENDER)
			{
				m_controllsLocked = true;
				//m_Player.GetInputController().OverrideAimChangeX(true,0);
				//m_Player.GetInputController().OverrideMovementSpeed(true,0);
			}
		}
		m_EmoteLockState = state;
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
		
		//surrender "state" switch
		if ( m_CurrentGestureID == ID_EMOTE_SURRENDER )
		{
			m_IsSurrendered = !m_IsSurrendered;
			if ( !m_IsSurrendered )
			{
				SetEmoteLockState(false);
			}
			else
			{
				SetEmoteLockState(true);
			}
		}
		
		if ( m_IsSurrendered )
		{
			if (m_BelayedEmote)
			{
				m_BelayedEmoteSlot = -1;
				m_BelayedEmote = false;
			}
			return;
		}
		
		SetEmoteLockState(false);
		
		if ( m_BelayedEmote )
		{
			if (!m_IsSurrendered)
			{
				PickEmote(m_BelayedEmoteSlot);
				//PlayEmote(m_BelayedEmoteSlot);
			}
			m_BelayedEmoteSlot = -1;
			m_BelayedEmote = false;
		}

		//! back to the default - shoot from camera - if not set already
		if (!m_Player.IsShootingFromCamera()) m_Player.OverrideShootFromCamera(true);
	}


};
*/