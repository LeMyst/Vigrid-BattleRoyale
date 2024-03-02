modded class CarScript
{
    override void EEOnCECreate()
    {
        // Fill the car to max
        Fill( CarFluid.FUEL, GetFluidCapacity( CarFluid.FUEL ) );
        Fill( CarFluid.COOLANT, GetFluidCapacity( CarFluid.COOLANT ) );
        Fill( CarFluid.OIL, GetFluidCapacity( CarFluid.OIL ) );
    }
}
