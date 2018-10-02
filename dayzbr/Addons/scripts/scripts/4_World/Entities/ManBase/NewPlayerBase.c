modded class PlayerBase
{
	
	float timeTillNextHealTick = 0;
	float timeTillNextDmgTick = 0;
	
	ref BattleRoyaleBase BR_BASE;
	
	void OnScheduledTick(float deltaTime)
	{
		if( !IsPlayerSelected() || !IsAlive() ) return;
		if( m_ModifiersManager ) m_ModifiersManager.OnScheduledTick(deltaTime);
		if( m_NotifiersManager ) m_NotifiersManager.OnScheduledTick();
		if( m_TrasferValues ) m_TrasferValues.OnScheduledTick(deltaTime);
		if( m_DisplayStatus ) m_DisplayStatus.OnScheduledTick();
		
		if( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
		{
			if(BR_BASE)
				BR_BASE.OnPlayerTick(this, deltaTime);
		}
	}
	
	override void EEKilled( Object killer )
	{
		Print("EEKilled, you have died");
		if( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_CLIENT )
		{
			// @NOTE: this branch does not happen, EEKilled is called only on server
			if( GetGame().GetPlayer() == this )
			{
				super.EEKilled( killer );
			}
			if (GetHumanInventory().GetEntityInHands())
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ServerDropEntity,1000,false,( GetHumanInventory().GetEntityInHands() ));
		}
		else if( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)//server
		{
			
			
			if( GetBleedingManager() ) delete GetBleedingManager();
			if( GetHumanInventory().GetEntityInHands() )
				GetGame().GetCallQueue(CALL_CATEGORY_GAMEPLAY).CallLater(ServerDropEntity,1000,false,( GetHumanInventory().GetEntityInHands() ));
		}
		
		if ( GetSoftSkillManager() )
		{
			delete GetSoftSkillManager();
		} 
		
		GetStateManager().OnPlayerKilled();
		
		// kill character in database
		if (GetHive())
		{
			GetHive().CharacterKill(this);
		}
		
		if( GetInstanceType() == DayZPlayerInstanceType.INSTANCETYPE_SERVER)
		{
			if(BR_BASE)
				BR_BASE.OnPlayerKilled(this,killer);
		}
	}
}