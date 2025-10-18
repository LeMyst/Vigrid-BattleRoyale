#ifdef SERVER
modded class BoatScript
{
    override void EEOnCECreate()
    {
        // Fill the boat with random fuel amount (30-100%)
        Fill( BoatFluid.FUEL, GetFluidCapacity( BoatFluid.FUEL ) * Math.RandomFloatInclusive(0.30, 1) );
    }
};
#endif