#ifdef SERVER
modded class ItemBase
{
	// Thanks to @incinatus for this
	override void EEOnCECreate()
	{
		super.EEOnCECreate();

		if( HasEnergyManager() )
		{
			ItemBase item;
			item = ItemBase.Cast( GetInventory().CreateInInventory( "Battery9V" ) );  // Try to spawn a 9V battery in the inventory
			ApplyRandomHealthAndEnergy(item);  // Apply random health and quantity
			item = ItemBase.Cast( GetInventory().CreateInInventory( "CarBattery" ) );  // Try to spawn a car battery in the inventory
			ApplyRandomHealthAndEnergy(item);  // Apply random health and quantity
		}
	}

	static const float HEALTH_MIN_FACTOR = 0.66;  // Minimum health as a fraction of max health
	static const float ENERGY_MIN = 0.5;         // Minimum energy level
	static const float ENERGY_MAX = 0.9;         // Maximum energy level

	void ApplyRandomHealthAndEnergy(ItemBase item)
	{
		if (item)
		{
			item.SetHealth(Math.RandomIntInclusive(item.GetMaxHealth() * HEALTH_MIN_FACTOR, item.GetMaxHealth()));  // Random health
			item.GetCompEM().SetEnergy0To1(Math.RandomFloatInclusive(ENERGY_MIN, ENERGY_MAX));  // Random energy level
		}
	}
}