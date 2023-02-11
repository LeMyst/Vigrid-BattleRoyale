modded class CarScript
{
	override void EEOnCECreate()
	{
	    // Fill the car to max fuel
		Fill( CarFluid.FUEL, GetFluidCapacity( CarFluid.FUEL ) );
	}
}
