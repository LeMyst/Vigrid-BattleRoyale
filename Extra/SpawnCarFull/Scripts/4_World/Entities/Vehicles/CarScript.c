#ifdef SERVER
modded class CarScript
{
    override void EEOnCECreate()
    {
        // Fill the car to max
        Fill( CarFluid.FUEL, GetFluidCapacity( CarFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
        Fill( CarFluid.COOLANT, GetFluidCapacity( CarFluid.COOLANT ) );
        Fill( CarFluid.OIL, GetFluidCapacity( CarFluid.OIL ) );
    }
};
#endif