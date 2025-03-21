#ifdef SERVER
modded class ItemBase
{
	// Thanks to @incinatus for this
	override void EEOnCECreate()
	{
		super.EEOnCECreate();

		if( HasEnergyManager() )
		{
			GetInventory().CreateInInventory( "Battery9V" );  // Try to spawn a 9V battery in the inventory
			GetInventory().CreateInInventory( "CarBattery" );  // Try to spawn a car battery in the inventory
			// TODO: Random health and quantity for better experience
		}
	}
}