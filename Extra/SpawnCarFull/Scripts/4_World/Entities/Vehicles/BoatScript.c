#ifdef SERVER
modded class BoatScript
{
    override void EEOnCECreate()
    {
        // Fill the car to max
        Fill( BoatFluid.FUEL, GetFluidCapacity( BoatFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
    }
};
#endif