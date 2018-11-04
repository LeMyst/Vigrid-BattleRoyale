class CAContinuousQuantityMonstaStamina : CAContinuousQuantityEdible
{
	void CAContinuousQuantityEdible( float quantity_used_per_second, float time_to_repeat )
	{
		m_QuantityUsedPerSecond = quantity_used_per_second;
		m_DefaultTimeToRepeat = time_to_repeat;
		m_InitItemQuantity = -1;
	}
	
	override void CalcAndSetQuantity( ActionData action_data )
	{	
		//Print("Munch!");
		if ( m_SpentUnits )
		{
			m_SpentUnits.param1 = m_SpentQuantity;
			SetACData(m_SpentUnits);
			
			m_SpentQuantityTotal += m_SpentQuantity;
		}
		//m_SpentQuantity = Math.Floor(m_SpentQuantity);

		PlayerBase ntarget = PlayerBase.Cast( action_data.m_Target.GetObject() );
		if ( ntarget )
		{
			if ( GetGame().IsServer() ) 
			{
				ConsumeBRItem(ntarget,action_data.m_MainItem,m_SpentQuantity);
			}
		}
		else
		{
			if ( GetGame().IsServer() ) 
			{
				//action_data.m_MainItem.Consume(action_data.m_Player, m_SpentQuantity);
				ConsumeBRItem(action_data.m_Player,action_data.m_MainItem,m_SpentQuantity);
			}
		}
	}
	//TODO: maybe structure all of this data better?
	void ConsumeBRItem(PlayerBase target, ItemBase item, float amount)
	{
		item.AddQuantity(-amount,false,false); //take away amount we have consumed
		
		//increase our stamina (but do not go over limit)
		float maxStamina = target.GetStatStamina().GetMax();
		
		float stamina_change = (amount / 100) * maxStamina;
		
		float currentStamina = target.GetStatStamina().Get();
		
		float newStamina = Math.Min(currentStamina + stamina_change, maxStamina);
		
		target.GetStatStamina().Set(newStamina);
	}
	
};

class ActionDrinkMonstaStaminaCB : ActionContinuousBaseCB
{
	private const float QUANTITY_USED_PER_SEC2 = 50; //amount used every 2 seconds 
	
	override void CreateActionComponent()
	{
		m_ActionData.m_ActionComponent = new CAContinuousQuantityMonstaStamina(QUANTITY_USED_PER_SEC2,UATimeSpent.DEFAULT);
	}
};

//Healing monster energy
class ActionDrinkMonstaStamina: ActionDrink
{
	void ActionDrinkMonsta()
	{
		m_CallbackClass = ActionDrinkMonstaStaminaCB;
		m_CommandUID = DayZPlayerConstants.CMD_ACTIONMOD_DRINKCAN;
		m_CommandUIDProne = DayZPlayerConstants.CMD_ACTIONFB_DRINKCAN;
	}
	
	override int GetType()
	{
		return AT_DRINK_MONSTASTAMINA;
	}
	
	override string GetText()
	{
		return "Drink Monsta";
	}
};